/*
 * Copyright (C) 2021-2022 CodeShop B.V.
 *
 * This file is part of the cuti library.
 *
 * The cuti library is free software: you can redistribute it and/or
 * modify it under the terms of version 2.1 of the GNU Lesser General
 * Public License as published by the Free Software Foundation.
 *
 * The cuti library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See version
 * 2.1 of the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of version 2.1 of the GNU Lesser
 * General Public License along with the cuti library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */
 
#include "select_selector.hpp"

#if CUTI_HAS_SELECT_SELECTOR

#include "list_arena.hpp"
#include "system_error.hpp"

#include <algorithm>
#include <cassert>
#include <utility>

#ifdef _WIN32

#include <winsock2.h>

#else

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>

#endif

namespace cuti
{

namespace // anonymous
{

#ifdef _WIN32

/*
 * As a hack to avoid the O(N^2) scalability of building an fd_set
 * with FD_SET (where N is the number of sockets to monitor) and to
 * escape from the limitations of FD_SETSIZE, we provide a one-shot
 * wrapper for winsock's fd_set type.
 */
struct fd_set_t
{
  fd_set_t()
  : inline_impl_()
  , current_impl_(&inline_impl_)
  , impl_holder_(nullptr)
  { }

  fd_set_t(fd_set_t const&) = delete;
  fd_set_t& operator=(fd_set_t const&) = delete;
  
  /*
   * Adds a socket.  Assumes no duplicate sockets, and that select()
   * was not called yet.
   */
  void add(int fd)
  {
    bool added = current_impl_->add(fd);
    if(!added)
    {
      // current_impl_ is full; get a bigger one
      impl_holder_.reset(current_impl_->create_successor());
      current_impl_ = impl_holder_.get();
      added = current_impl_->add(fd);
      assert(added);
    }
  }

  /*
   * Returns a pointer to the current impersonator, yet another
   * wannabe fd_set.
   */
  fd_set* as_fd_set()
  {
    return current_impl_->as_fd_set();
  }

  /*
   * Tells if fd was selected, assuming select() was called.  When
   * there are only a few ready sockets (which is what we optimize for
   * in C10K land), we're close to O(N) complexity; if all sockets are
   * ready, then checking each of them does lead us to O(N^2).
   */
  bool contains(int fd)
  {
    return FD_ISSET(fd, current_impl_->as_fd_set());
  }

private :
  struct impl_t
  {
    impl_t()
    { }

    impl_t(impl_t const&) = delete;
    impl_t& operator=(impl_t const&) = delete;

    virtual bool add(SOCKET socket) = 0;
    virtual impl_t* create_successor() const = 0;
    virtual fd_set* as_fd_set() = 0;

    virtual ~impl_t()
    { }
  };

  template<std::size_t FdSetSize>
  struct impl_instance_t : impl_t
  {
    impl_instance_t()
    : impl_t()
    {
      impersonator_.fd_count = 0;
    }

    template<std::size_t RhsFdSetSize>
    impl_instance_t(impl_instance_t<RhsFdSetSize> const& rhs)
    : impl_t()
    {
      static_assert(RhsFdSetSize <= FdSetSize);
      assert(rhs.impersonator_.fd_count <= RhsFdSetSize);

      this->impersonator_.fd_count = rhs.impersonator_.fd_count;
      std::copy(rhs.impersonator_.fd_array,
                rhs.impersonator_.fd_array + rhs.impersonator_.fd_count,
                this->impersonator_.fd_array);
    }

    bool add(SOCKET socket) override
    {
      if(impersonator_.fd_count >= FdSetSize)
      {
        return false;
      }

      impersonator_.fd_array[impersonator_.fd_count] = socket;
      ++impersonator_.fd_count;
      return true;
    }
        
    impl_t* create_successor() const override
    {
      impl_t* result = nullptr;

      constexpr std::size_t MaxFdSetSize = 10000; // C10K! :-)
      if constexpr(FdSetSize >= MaxFdSetSize)
      {
        system_exception_builder_t builder;
        builder << "select_selector(): maximum FD_SETSIZE(" <<
          FdSetSize << ") exceeded";
        builder.explode();
      }
      else
      {
        constexpr std::size_t NextFdSetSize =
          FdSetSize + FdSetSize / 2 + FD_SETSIZE;
        result = new impl_instance_t<NextFdSetSize>(*this);
      }

      return result;
    }

    fd_set* as_fd_set() override
    {
      // just one little lie, but we could say there's a common
      // initial prefix, so have mercy please!
      return reinterpret_cast<fd_set*>(&impersonator_);
    }

    // should be very close to struct fd_set from <winsock2.h>
    struct
    {
      u_int fd_count;
      SOCKET fd_array[FdSetSize];
    }
    impersonator_;
  };

  impl_instance_t<FD_SETSIZE> inline_impl_;
  impl_t* current_impl_;
  std::unique_ptr<impl_t> impl_holder_;
};

#else

struct fd_set_t
{
  fd_set_t()
  { FD_ZERO(&set_); }

  fd_set_t(fd_set_t const&) = delete;
  void operator=(fd_set_t const&) = delete;

  void add(int fd)
  {
    assert(fd >= 0);
    if(fd >= FD_SETSIZE)
    {
      throw system_exception_t("select_selector: FD_SETSIZE overflow");
    }
    FD_SET(fd, &set_);
  }

  fd_set* as_fd_set()
  { return &set_; }

  bool contains(int fd)
  { return FD_ISSET(fd, &set_); }
  
private :
  fd_set set_;
};

#endif // !_WIN32

struct select_selector_t : selector_t
{
  select_selector_t()
  : selector_t()
  , registrations_()
  , watched_list_(registrations_.add_list())
  , pending_list_(registrations_.add_list())
  { }

  int call_when_writable(int fd, callback_t callback) override
  {
    return make_ticket(fd, event_t::writable, std::move(callback));
  }

  void cancel_when_writable(int ticket) noexcept override
  {
    remove_registration(ticket);
  }

  int call_when_readable(int fd, callback_t callback) override
  {
    return make_ticket(fd, event_t::readable, std::move(callback));
  }

  void cancel_when_readable(int ticket) noexcept override
  {
    remove_registration(ticket);
  }

  bool has_work() const noexcept override
  {
    return !registrations_.list_empty(watched_list_) ||
           !registrations_.list_empty(pending_list_);
  }

  callback_t select(duration_t timeout) override
  {
    assert(this->has_work());

    if(registrations_.list_empty(pending_list_))
    {
      fd_set_t infds;
      fd_set_t outfds;

      int nfds = 0;

      for(int ticket = registrations_.first(watched_list_);
          ticket != registrations_.last(watched_list_);
          ticket = registrations_.next(ticket))
      {
        auto const& registration = registrations_.value(ticket);
        int fd = registration.fd_;

        switch(registration.event_)
        {
        case event_t::writable :
          outfds.add(fd);
          break;
        case event_t::readable :
          infds.add(fd);
          break;
        default :
          assert(!"expected event type");
          break;
        }

        if(fd >= nfds)
        {
          nfds = fd + 1;
        }
      }

      struct timeval tv;
      struct timeval* ptv = nullptr;
      if(timeout >= duration_t::zero())
      {
        int millis = timeout_millis(timeout);
        assert(millis >= 0);

        tv.tv_sec = millis / 1000;
        tv.tv_usec = (millis % 1000) * 1000;
        ptv = &tv;
      }

      int count = ::select(nfds,
        infds.as_fd_set(), outfds.as_fd_set(), nullptr, ptv);
      if(count < 0)
      {
        int cause = last_system_error();
#ifdef _WIN32
        throw system_exception_t("select() failure", cause);
#else
        if(cause != EINTR)
        {
          throw system_exception_t("select() failure", cause);
        }
        count = 0;
#endif
      }

      int ticket = registrations_.first(watched_list_);
      while(count > 0 && ticket != registrations_.last(watched_list_))
      {
        int next = registrations_.next(ticket);
        auto const& registration = registrations_.value(ticket);
        int fd = registration.fd_;

        bool is_set = false;
        switch(registration.event_)
        {
        case event_t::writable :
          is_set = outfds.contains(fd);
          break;
        case event_t::readable :
          is_set = infds.contains(fd);
          break;
        default :
          assert(!"expected event type");
          break;
        }

        if(is_set)
        {
          registrations_.move_element_before(
            registrations_.last(pending_list_), ticket);
          --count;
        }

        ticket = next;
      }

      assert(count == 0);
    }

    if(registrations_.list_empty(pending_list_))
    {
      return nullptr;
    }

    int ticket = registrations_.first(pending_list_);
    auto result = std::move(registrations_.value(ticket).callback_);
    remove_registration(ticket);
    return result;
  }

private :
  int make_ticket(int fd, event_t event, callback_t callback)
  {
    assert(callback != nullptr);

    int ticket = registrations_.add_element_before(
      registrations_.last(watched_list_),
      fd, event, std::move(callback));

    return ticket;
  }

  void remove_registration(int ticket) noexcept
  {
    registrations_.remove_element(ticket);
  }

private :
  struct registration_t
  {
    registration_t(int fd, event_t event, callback_t callback)
    : fd_(fd)
    , event_(event)
    , callback_(std::move(callback))
    { }

    int fd_;
    event_t event_;
    callback_t callback_;
  };

  list_arena_t<registration_t> registrations_;
  int const watched_list_;
  int const pending_list_;
};

} // anonymous

std::unique_ptr<selector_t> create_select_selector()
{
  return std::make_unique<select_selector_t>();
}

} // cuti

#endif // CUTI_HAS_SELECT_SELECTOR
