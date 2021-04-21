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

#include "poll_selector.hpp"

#ifndef _WIN32

#include "list_arena.hpp"
#include "scoped_guard.hpp"
#include "system_error.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <utility>
#include <vector>

#include <errno.h>
#include <poll.h>

namespace cuti
{

namespace // anonymous
{

// a pollfd with an fd of -1 is skipped by poll()
pollfd const inactive_pollfd = { -1, 0, 0 };

struct poll_selector_t : selector_t
{
  poll_selector_t()
  : selector_t()
  , callbacks_()
  , watched_list_(callbacks_.add_list())
  , pending_list_(callbacks_.add_list())
  , pollfds_()
  { }

  int call_when_writable(int fd, callback_t callback) override
  {
    return make_ticket(fd, POLLOUT, std::move(callback));
  }

  int call_when_readable(int fd, callback_t callback) override
  {
    return make_ticket(fd, POLLIN, std::move(callback));
  }

  void cancel_callback(int ticket) noexcept override
  {
    assert(ticket >= 0);
    assert(static_cast<std::size_t>(ticket) < pollfds_.size());

    pollfds_[ticket] = inactive_pollfd;
    callbacks_.remove_element(ticket);
  }

  bool has_work() const noexcept override
  {
    return !callbacks_.list_empty(watched_list_) ||
           !callbacks_.list_empty(pending_list_);
  }

  callback_t select(timeout_t timeout) override
  {
    assert(this->has_work());

    if(callbacks_.list_empty(pending_list_))
    {
      int count = ::poll(
        pollfds_.data(), pollfds_.size(), poll_timeout(timeout));
      if(count < 0)
      {
        int cause = last_system_error();
        if(cause != EINTR)
        {
          throw system_exception_t("poll() failure", cause);
        }
        count = 0;
      }

      int ticket = callbacks_.first(watched_list_);
      while(count > 0 && ticket != callbacks_.last(watched_list_))
      {
        int next = callbacks_.next(ticket);

        assert(ticket >= 0);
        assert(static_cast<std::size_t>(ticket) < pollfds_.size());
        if(pollfds_[ticket].revents != 0)
        {
          pollfds_[ticket] = inactive_pollfd;
          callbacks_.move_element_before(
            callbacks_.last(pending_list_), ticket);
          --count;
        }

        ticket = next;
      }

      assert(count == 0);
    }
            
    callback_t result = nullptr;
    if(!callbacks_.list_empty(pending_list_))
    {
      int ticket = callbacks_.first(pending_list_);
      result = std::move(callbacks_.value(ticket));
      callbacks_.remove_element(ticket);
    }
    return result;
  }
    
private :
  int make_ticket(int fd, int events, callback_t callback)
  {
    assert(callback != nullptr);

    // Obtain a ticket, guarding it for exceptions
    int ticket = callbacks_.add_element_before(
      callbacks_.last(watched_list_), std::move(callback));
    auto ticket_guard =
      make_scoped_guard([&] { callbacks_.remove_element(ticket); });

    std::size_t min_size = static_cast<unsigned int>(ticket) + 1;
    while(pollfds_.size() < min_size)
    {
      // use push_back(), not resize(), for amortized O(1)
      pollfds_.push_back(inactive_pollfd);
    }

    pollfds_[ticket].fd = fd;
    pollfds_[ticket].events = events;
    pollfds_[ticket].revents = 0;

    ticket_guard.dismiss();
    return ticket;
  }

  static int poll_timeout(timeout_t timeout)
  {
    static timeout_t const zero = timeout_t::zero();
    static int const max_poll_timeout = 30000;

    int result;

    if(timeout < zero)
    {
      result = -1;
    }
    else if(timeout == zero)
    {
      result = 0;
    }
    else // timeout > zero
    {
      auto count = std::chrono::duration_cast<std::chrono::milliseconds>(
        timeout).count();
      if(count < 1)
      {
        result = 1; // prevent spinloop
      }
      else if(count < max_poll_timeout)
      {
        result = static_cast<int>(count);
      }
      else
      {
        result = max_poll_timeout;  
      }
    }

    return result;
  }

private :
  list_arena_t<callback_t> callbacks_;
  int const watched_list_;
  int const pending_list_;
  std::vector<pollfd> pollfds_; // indexed by the ids from callbacks_
};
  
} // anonymous

std::unique_ptr<selector_t> create_poll_selector()
{
  return std::make_unique<poll_selector_t>();
}

} // cuti

#endif // !_WIN32
