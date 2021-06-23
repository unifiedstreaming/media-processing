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

#if CUTI_HAS_KQUEUE_SELECTOR

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

struct kqueue_t
{
  kqueue_t()
  : fd_(::kqueue())
  {
    if(fd_ == -1)
    {
      int cause = last_system_error();
      throw system_exception_t("kqueue() failure", cause);
    }
  }

  ~kqueue_t()
  {
    ::close(fd_);
  }

  kqueue_t(kqueue_t const&) = delete;
  kqueue_t& operator=(kqueue_t const&) = delete;

  int add_oneshot(int fd, int filter, void* udata) const
  {
    struct kevent kev;
    EV_SET(&kev, fd, filter, EV_ADD | EV_ONESHOT, 0, 0, udata);
    return ::kevent(fd_, &kev, 1, nullptr, 0, nullptr);
  }

  int remove(int fd, int filter, void* udata) const
  {
    struct kevent kev;
    EV_SET(&kev, fd, filter, EV_DELETE, 0, 0, udata);
    return ::kevent(fd_, &kev, 1, nullptr, 0, nullptr);
  }

  int get(struct kevent *eventlist, int nevents,
          const struct timespec *timeout) const
  {
    return ::kevent(fd_, nullptr, 0, eventlist, nevents, timeout);
  }

private:
  int fd_;
};

struct kqueue_selector_t : selector_t
{
  kqueue_selector_t()
  : selector_t()
  , registrations_()
  , watched_list_(registrations_.add_list())
  , pending_list_(registrations_.add_list())
  , kqueue_()
  {
  }

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
      struct timespec ts;
      struct timespec* pts = nullptr;
      if(timeout >= duration_t::zero())
      {
        int millis = timeout_millis(timeout);
        assert(millis >= 0);

        ts.tv_sec = millis / 1000;
        ts.tv_nsec = (millis % 1000) * 1000000;
        pts = &ts;
      }

      int const num_events = 16;
      struct kevent kevents[num_events];
      int count = kqueue_.get(kevents, num_events, pts);
      if(count < 0)
      {
        int cause = last_system_error();
        if(cause != EINTR)
        {
          throw system_exception_t("kevent() failed to retrieve events", cause);
        }
        count = 0;
      }

      for(int i = 0; i < count; ++i)
      {
        // Two-stage cast, to avoid the compiler choking on "error: cast from
        // pointer to smaller type 'int' loses information"
        auto lticket = reinterpret_cast<intptr_t>(kevents[i].udata);
        assert(lticket >= 0 && lticket <= std::numeric_limits<int>::max());
        auto ticket = static_cast<int>(lticket);

        auto& registration = registrations_.value(ticket);
        assert(registration.fd_ == kevents[i].ident);
        assert((registration.event_ == event_t::writable &&
                kevents[i].filter == EVFILT_WRITE) ||
               (registration.event_ == event_t::readable &&
                kevents[i].filter == EVFILT_READ));

        registration.fd_ = -1;
        registrations_.move_element_before(
          registrations_.last(pending_list_), ticket);
      }
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
  int make_ticket(int fd, event_t event, callback_t callback)
  {
    assert(fd != -1);
    assert(callback != nullptr);

    // Obtain a ticket, guarding it for exceptions
    int ticket = registrations_.add_element_before(
      registrations_.last(watched_list_),
      registration_t(fd, event, std::move(callback)));
    auto ticket_guard =
      make_scoped_guard([&] { registrations_.remove_element(ticket); });

    // Add an event to monitor to the kqueue.
    int count = kqueue_.add_oneshot(fd,
      event == event_t::writable ? EVFILT_WRITE : EVFILT_READ,
      reinterpret_cast<void*>(ticket));
    if(count == -1)
    {
      int cause = last_system_error();
      throw system_exception_t("kevent() failed to add event", cause);
    }

    ticket_guard.dismiss();
    return ticket;
  }

  void remove_registration(int ticket) noexcept
  {
    assert(ticket >= 0);

    auto const& registration = registrations_.value(ticket);
    if(registration.fd_ != -1)
    {
      // Remove the event from the kqueue.
      int count = kqueue_.remove(registration.fd_,
        registration.event_ == event_t::writable ? EVFILT_WRITE : EVFILT_READ,
        reinterpret_cast<void*>(ticket));

      static_cast<void>(count);
      assert(count == 0);
    }

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
  kqueue_t kqueue_;
};

} // anonymous

std::unique_ptr<selector_t> create_kqueue_selector()
{
  return std::make_unique<kqueue_selector_t>();
}

} // cuti

#endif // CUTI_HAS_KQUEUE_SELECTOR
