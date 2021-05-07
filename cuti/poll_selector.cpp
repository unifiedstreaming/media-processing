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

#if CUTI_HAS_POLL_SELECTOR

#include "list_arena.hpp"
#include "scoped_guard.hpp"
#include "system_error.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <limits>
#include <utility>
#include <vector>

#include <errno.h>
#include <poll.h>
#include <sys/resource.h>

namespace cuti
{

namespace // anonymous
{

// a pollfd with an fd of -1 is skipped by poll()
pollfd const inactive_pollfd = { -1, 0, 0 };

std::size_t max_pollfds_size()
{
  struct rlimit rl;

  int r = getrlimit(RLIMIT_NOFILE, &rl);
  if(r == -1)
  {
    int cause = last_system_error();
    throw system_exception_t("getrlimit(RLIMIT_NOFILE) failure", cause);
  }

  auto result = std::numeric_limits<std::size_t>::max();
  if(rl.rlim_cur != RLIM_INFINITY)
  {
    assert(rl.rlim_cur >= 0);
    result = rl.rlim_cur;
  }
  return result;
}

struct poll_selector_t : io_selector_t
{
  poll_selector_t()
  : io_selector_t()
  , callbacks_()
  , watched_list_(callbacks_.add_list())
  , pending_list_(callbacks_.add_list())
  , max_pollfds_size_(max_pollfds_size())
  , pollfds_()
  { }

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
        pollfds_.data(), pollfds_.size(), timeout_millis(timeout));
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
  int do_call_when_writable(int fd, callback_t callback) override
  {
    return make_ticket(fd, POLLOUT, std::move(callback));
  }

  void do_cancel_when_writable(int cancellation_ticket) noexcept override
  {
    cancel_ticket(cancellation_ticket);
  }

  int do_call_when_readable(int fd, callback_t callback) override
  {
    return make_ticket(fd, POLLIN, std::move(callback));
  }

  void do_cancel_when_readable(int cancellation_ticket) noexcept override
  {
    cancel_ticket(cancellation_ticket);
  }

  int make_ticket(int fd, int events, callback_t callback)
  {
    assert(fd != -1);
    assert(callback != nullptr);

    // Obtain a ticket, guarding it for exceptions
    int ticket = callbacks_.add_element_before(
      callbacks_.last(watched_list_), std::move(callback));
    auto ticket_guard =
      make_scoped_guard([&] { callbacks_.remove_element(ticket); });

    std::size_t min_size = static_cast<unsigned int>(ticket) + 1;
    if(min_size > max_pollfds_size_)
    {
      system_exception_builder_t builder;
      builder << "poll_selector: maximum number of pollfds (" <<
        max_pollfds_size_ << ") exceeded";
      builder.explode();
    }

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

  void cancel_ticket(int ticket) noexcept
  {
    assert(ticket >= 0);
    assert(static_cast<std::size_t>(ticket) < pollfds_.size());

    pollfds_[ticket] = inactive_pollfd;
    callbacks_.remove_element(ticket);
  }

private :
  list_arena_t<callback_t> callbacks_;
  int const watched_list_;
  int const pending_list_;
  std::size_t const max_pollfds_size_;
  std::vector<pollfd> pollfds_; // indexed by the ids from callbacks_
};

} // anonymous

std::unique_ptr<io_selector_t> create_poll_selector()
{
  return std::make_unique<poll_selector_t>();
}

} // cuti

#endif // CUTI_HAS_POLL_SELECTOR
