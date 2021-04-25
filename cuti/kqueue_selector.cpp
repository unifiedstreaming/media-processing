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

#include "kqueue_selector.hpp"

#if defined(__APPLE__) || defined(__FreeBSD__)

#include "list_arena.hpp"
#include "scoped_guard.hpp"
#include "system_error.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <limits>
#include <utility>
#include <vector>

#include <sys/event.h>
#include <unistd.h>

namespace cuti
{

namespace // anonymous
{

// a kqueuefd with an fd of -1 is skipped by kqueue()
//kqueuefd const inactive_kqueuefd = { -1, 0, 0 };

struct kqueue_selector_t : selector_t
{
  kqueue_selector_t()
  : selector_t()
  , registrations_()
  , watched_list_(registrations_.add_list())
  , pending_list_(registrations_.add_list())
  , kqueue_fd_(make_kqueue())
  { }

  ~kqueue_selector_t()
  {
    if(kqueue_fd_ != -1)
    {
      ::close(kqueue_fd_);
    }
  }

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
      std::vector<struct kevent> kevents;

      for(int ticket = registrations_.first(watched_list_);
          ticket != registrations_.last(watched_list_);
          ticket = registrations_.next(ticket))
      {
        auto const& registration = registrations_.value(ticket);
        int fd = registration.fd_;

        switch(registration.event_)
        {
        case event_t::writable :
          kevents.push_back(make_kevent(fd, EVFILT_WRITE, EV_ADD | EV_ONESHOT));
          break;
        case event_t::readable :
          kevents.push_back(make_kevent(fd, EVFILT_READ, EV_ADD | EV_ONESHOT));
          break;
        default :
          assert(!"expected event type");
          break;
        }
      }
      assert(!kevents.empty());

      auto ke_timeout = kevent_timeout(timeout);
      int count = ::kevent(kqueue_fd_, kevents.data(), kevents.size(),
                           kevents.data(), kevents.size(),
                           ke_timeout.tv_sec < 0 ? nullptr : &ke_timeout);
      if(count < 0)
      {
        int cause = last_system_error();
        if(cause != EINTR)
        {
          throw system_exception_t("kevent() failure", cause);
        }
        count = 0;
      }

      // kevent(2) returns the number of events placed in the eventlist, and
      // since we used the kevents vector for both input and output (this is
      // just fine), resize it now to the actual number.
      kevents.resize(count);

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
          is_set = match_kevent(kevents, fd, EVFILT_WRITE);
          break;
        case event_t::readable :
          is_set = match_kevent(kevents, fd, EVFILT_READ);
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
  static int make_kqueue()
  {
    int fd = ::kqueue();
    if(fd == -1)
    {
      int cause = last_system_error();
      throw system_exception_t("kqueue() failure", cause);
    }
    return fd;
  }

  static struct kevent make_kevent(int fd, int filter, int flags)
  {
    struct kevent result;
    EV_SET(&result, fd, filter, flags, 0, 0, nullptr);
    return result;
  }

  static bool match_kevent(std::vector<struct kevent> const& kevents, int fd,
                           int filter)
  {
    auto first = kevents.begin();
    auto last = kevents.end();
    auto cmp = [fd, filter](struct kevent const& kev)
    {
      return kev.ident == fd && kev.filter == filter;
    };
    return std::find_if(first, last, cmp) != last;
  }

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

  static struct timespec kevent_timeout(timeout_t timeout)
  {
    static timeout_t const zero = timeout_t::zero();
    static long long const nano = 1'000'000'000;
    static long long const max_kevent_timeout = 30 * nano;

    struct timespec result;

    if(timeout < zero)
    {
      result.tv_sec = -1;
      result.tv_nsec = 0;
    }
    else
    {
      auto count = std::chrono::duration_cast<std::chrono::nanoseconds>(
        timeout).count();
      if(count < 1 && timeout > zero)
      {
        count = 1; // prevent spinloop
      }
      else if(count > max_kevent_timeout)
      {
        count = max_kevent_timeout;
      }
      result.tv_sec = count / nano;
      result.tv_nsec = count % nano;
    }

    return result;
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
  int kqueue_fd_;
};

} // anonymous

std::unique_ptr<selector_t> create_kqueue_selector()
{
  return std::make_unique<kqueue_selector_t>();
}

} // cuti

#endif // __APPLE__ || __FreeBSD__
