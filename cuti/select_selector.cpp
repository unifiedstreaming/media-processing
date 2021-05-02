/*
 * Copyright (C) 2021 CodeShop B.V.
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
 * As a hack to avoid O(N^2) scalability when building an fd_set
 * (where N is the number of sockets to monitor) and escape from the
 * limitations of FD_SETSIZE, we provide a one-shot wrapper for
 * winsock's fd_set type.
 */
struct fd_set_t
{
  fd_set_t()
  : buffers_(&inline_buffer_)
  , n_buffers_(1)
  {
    buffers_->fd_set_.fd_count = 0;
  }
  
  fd_set_t(fd_set_t const&) = delete;
  void operator=(fd_set_t const&) = delete;

  /*
   * Add a socket to the set; assumes no duplicate sockets and that
   * select() has not been called yet.
   */
  void add(SOCKET socket)
  {
    if(buffers_->fd_set_.fd_count + 1 == n_buffers_ * (FD_SETSIZE + 1))
    {
      // allocate more buffers
      std::size_t new_n_buffers = n_buffers_;
      new_n_buffers += n_buffers_ / 2;
      new_n_buffers += 1;

      buffer_t* new_buffers = new buffer_t[new_n_buffers];
      std::copy(buffers_, buffers_ + n_buffers_, new_buffers);

      if(n_buffers_ != 1)
      {
        delete[] buffers_;
      }
      buffers_ = new_buffers;
      n_buffers_ = new_n_buffers;
    }

    SOCKET* sockets = buffers_->fd_set_.fd_array;
    sockets[buffers_->fd_set_.fd_count] = socket;
    ++buffers_->fd_set_.fd_count;
  }

  fd_set* get()
  {
    return &buffers_->fd_set_;
  }

  /*
   * Tells if socket is in the set; assumes select() has been called.
   * This still leads to O(N^2) complexity, but is only called while
   * there are selected sockets to be found.
   */
  bool contains(SOCKET socket)
  {
    return FD_ISSET(socket, &buffers_->fd_set_);
  }

  ~fd_set_t()
  {
    if(n_buffers_ != 1)
    {
      delete[] buffers_;
    }
  }

private :
  using socket_array_t = SOCKET[FD_SETSIZE + 1];
  static_assert(sizeof(fd_set) == sizeof(socket_array_t));

  union buffer_t
  {
    fd_set fd_set_;
    socket_array_t sockets_;
  };

  buffer_t inline_buffer_;
  buffer_t* buffers_;
  std::size_t n_buffers_; 
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

  fd_set* get()
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

  bool has_work() const noexcept override
  {
    return !registrations_.list_empty(watched_list_) ||
           !registrations_.list_empty(pending_list_);
  }

  callback_t select(timeout_t timeout) override
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
      if(timeout >= timeout_t::zero())
      {
        int millis = timeout_millis(timeout);
        assert(millis >= 0);

        tv.tv_sec = millis / 1000;
        tv.tv_usec = (millis % 1000) * 1000;
        ptv = &tv;
      }

      int count = ::select(nfds, infds.get(), outfds.get(), nullptr, ptv);
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

    callback_t result = nullptr;
    if(!registrations_.list_empty(pending_list_))
    {
      int ticket = registrations_.first(pending_list_);
      result = std::move(registrations_.value(ticket).callback_);
      remove_registration(ticket);
    }
    return result;
  }

private :
  int do_call_when_writable(int fd, callback_t callback) override
  {
    return make_ticket(fd, event_t::writable, std::move(callback));
  }

  void do_cancel_when_writable(int cancellation_ticket) noexcept override
  {
    remove_registration(cancellation_ticket);
  }

  int do_call_when_readable(int fd, callback_t callback) override
  {
    return make_ticket(fd, event_t::readable, std::move(callback));
  }

  void do_cancel_when_readable(int cancellation_ticket) noexcept override
  {
    remove_registration(cancellation_ticket);
  }

  int make_ticket(int fd, event_t event, callback_t callback)
  {
    assert(callback != nullptr);

    int ticket = registrations_.add_element_before(
      registrations_.last(watched_list_),
      registration_t(fd, event, std::move(callback)));

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
