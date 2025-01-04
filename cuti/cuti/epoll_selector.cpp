/*
 * Copyright (C) 2021-2025 CodeShop B.V.
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

#include "epoll_selector.hpp"

#if CUTI_HAS_EPOLL_SELECTOR

#include "list_arena.hpp"
#include "scoped_guard.hpp"
#include "system_error.hpp"

#include <cassert>
#include <limits>

#include <errno.h>
#include <poll.h>
#include <sys/epoll.h>
#include <unistd.h>

namespace cuti
{

namespace // anonymous
{

struct epoll_selector_t : selector_t
{
  epoll_selector_t()
  : selector_t()
  , registrations_()
  , watched_list_(registrations_.add_list())
  , pending_list_(registrations_.add_list())
  , writable_instance_()
  , readable_instance_()
  { }

  int call_when_writable(int fd, callback_t callback) override
  {
    return make_ticket(fd, event_t::writable, std::move(callback));
  }

  void cancel_when_writable(int ticket) noexcept override
  {
    cancel_ticket(ticket, writable_instance_.fd_);
  }

  int call_when_readable(int fd, callback_t callback) override
  {
    return make_ticket(fd, event_t::readable, std::move(callback));
  }

  void cancel_when_readable(int ticket) noexcept override
  {
    cancel_ticket(ticket, readable_instance_.fd_);
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
      pollfd pollfds[2];

      pollfds[0].fd = writable_instance_.fd_;
      pollfds[0].events = POLLIN;
      pollfds[0].revents = 0;

      pollfds[1].fd = readable_instance_.fd_;
      pollfds[1].events = POLLIN;
      pollfds[1].revents = 0;
   
      int count = ::poll(pollfds, 2, timeout_millis(timeout));
      if(count < 0)
      {
        int cause = last_system_error();
        if(cause != EINTR)
        {
          system_exception_builder_t builder;
          builder << "epoll_selector: poll() failure: " <<
            error_status_t(cause);
          builder.explode();
        }
        count = 0;
      }

      for(pollfd const* pfd = pollfds; count > 0 && pfd != pollfds + 2; ++pfd)
      {
        if(pfd->revents != 0)
        {
          drain_epoll_instance(pfd->fd);
          --count;
        }
      }
      assert(count == 0);
    }
    
    callback_t result = nullptr;
    if(!registrations_.list_empty(pending_list_))
    {
      int ticket = registrations_.first(pending_list_);
      result = std::move(registrations_.value(ticket).callback_);
      registrations_.remove_element(ticket);
    }
    return result;
  }
      
private :
  struct registration_t
  {
    registration_t(int fd, callback_t callback)
    : fd_(fd)
    , callback_(std::move(callback))
    { }

    int fd_;
    callback_t callback_;
  };

  struct epoll_instance_t
  {
    epoll_instance_t()
    : fd_(::epoll_create1(EPOLL_CLOEXEC))
    {
      if(fd_ == -1)
      {
        int cause = last_system_error();
        system_exception_builder_t builder;
        builder << "error creating epoll instance: " << error_status_t(cause);
        builder.explode();
      }
    }

    epoll_instance_t(epoll_instance_t const&) = delete;
    epoll_instance_t& operator=(epoll_instance_t const&) = delete;

    ~epoll_instance_t()
    { ::close(fd_); }

    int const fd_;
  };

  int make_ticket(int fd, event_t event, callback_t callback)
  {
    assert(fd != -1);
    assert(callback != nullptr);

    // Obtain a ticket, guarding it for exceptions
    int ticket = registrations_.add_element_before(
      registrations_.last(watched_list_), fd, std::move(callback));
    auto ticket_guard =
      make_scoped_guard([&] { registrations_.remove_element(ticket); });

    struct epoll_event epoll_event;
    epoll_event.data.u64 = ticket;

    int epoll_fd = -1;
    switch(event)
    {
    case event_t::writable :
      epoll_fd = writable_instance_.fd_;
      epoll_event.events = EPOLLOUT;
      break;
    case event_t::readable :
      epoll_fd = readable_instance_.fd_;
      epoll_event.events = EPOLLIN;
      break;
    default :
      assert(!"expected event type");
      break;
    }

    if(::epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &epoll_event) == -1)
    {
      int cause = last_system_error();
      system_exception_builder_t builder;
      builder << "error adding epoll event: " << error_status_t(cause);
      builder.explode();
    }

    ticket_guard.dismiss();
    return ticket;
  }

  void drain_epoll_instance(int epoll_fd)
  {
    struct epoll_event epoll_events[16];
    int count = ::epoll_wait(epoll_fd, epoll_events, 16, 0);
    if(count < 0)
    {
      int cause = last_system_error();
      if(cause != EINTR)
      {
        system_exception_builder_t builder;
        builder << "epoll_wait() failure: " << error_status_t(cause);
        builder.explode();
      }
      count = 0;
    }

    for(struct epoll_event const* epoll_event = epoll_events;
        epoll_event != epoll_events + count;
        ++epoll_event)
    {
      static unsigned int max_ticket = std::numeric_limits<int>::max();
      static_cast<void>(max_ticket); // suppress NDEBUG mode warning

      assert(epoll_event->data.u64 >= 0);
      assert(epoll_event->data.u64 <= max_ticket);

      int ticket = static_cast<int>(epoll_event->data.u64);
      delete_epoll_event(registrations_.value(ticket), epoll_fd);
      registrations_.move_element_before(
        registrations_.last(pending_list_), ticket);
    }
  }
  
  void cancel_ticket(int ticket, int epoll_fd) noexcept
  {
    assert(ticket >= 0);
    assert(ticket <= std::numeric_limits<int>::max());

    auto& registration = registrations_.value(ticket);
    if(registration.fd_ != -1)
    {
      delete_epoll_event(registration, epoll_fd);
    }

    registrations_.remove_element(ticket);
  }
    
  void delete_epoll_event(registration_t& registration, int epoll_fd) noexcept
  {
    assert(registration.fd_ != -1);
    if(::epoll_ctl(epoll_fd, EPOLL_CTL_DEL, registration.fd_, nullptr) == -1)
    {
      assert(!"success deleting epoll event");
    }

    registration.fd_ = -1;
  }
    
private :
  list_arena_t<registration_t> registrations_;
  int const watched_list_;
  int const pending_list_;
  epoll_instance_t writable_instance_;
  epoll_instance_t readable_instance_;
};
    
} // anonymous

std::unique_ptr<selector_t> create_epoll_selector()
{
  return std::make_unique<epoll_selector_t>();
}

} // cuti

#endif // CUTI_HAS_EPOLL_SELECTOR
