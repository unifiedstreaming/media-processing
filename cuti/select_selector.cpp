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

#include <cassert>
#include <utility>

#ifdef _WIN32

#ifndef FD_SETSIZE
#define FD_SETSIZE 512
#endif

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

struct select_selector_t : selector_t
{
  select_selector_t()
  : selector_t()
  , registrations_()
  , watched_list_(registrations_.add_list())
  , pending_list_(registrations_.add_list())
#ifdef _WIN32
  , n_writables_(0)
  , n_readables_(0)
#endif
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
      fd_set infds;
      FD_ZERO(&infds);

      fd_set outfds;
      FD_ZERO(&outfds);

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
          FD_SET(fd, &outfds);
          break;
        case event_t::readable :
          FD_SET(fd, &infds);
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

      int count = ::select(nfds, &infds, &outfds, nullptr, ptv);
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
          is_set = FD_ISSET(fd, &outfds);
          break;
        case event_t::readable :
          is_set = FD_ISSET(fd, &infds);
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

    bool overflow = false;

#ifdef _WIN32
    switch(event)
    {
    case event_t::writable :
      overflow = n_writables_ >= FD_SETSIZE;
      break;
    case event_t::readable :
      overflow = n_readables_ >= FD_SETSIZE;
      break;
    default :
      assert(!"expected event type");
      break;
    }
#else
    assert(fd >= 0);
    overflow = fd >= FD_SETSIZE;
#endif

    if(overflow)
    {
      throw system_exception_t("select_selector: FD_SETSIZE overflow");
    }

    int ticket = registrations_.add_element_before(
      registrations_.last(watched_list_),
      registration_t(fd, event, std::move(callback)));

#ifdef _WIN32
    switch(event)
    {
    case event_t::writable :
      ++n_writables_;
      break;
    case event_t::readable :
      ++n_readables_;
      break;
    default :
      assert(!"expected event type");
      break;
    }
#endif

    return ticket;
  }

  void remove_registration(int ticket) noexcept
  {
#ifdef _WIN32
    switch(registrations_.value(ticket).event_)
    {
    case event_t::writable :
      assert(n_writables_ > 0);
      --n_writables_;
      break;
    case event_t::readable :
      assert(n_readables_ > 0);
      --n_readables_;
      break;
    default :
      assert(!"expected event type");
      break;
    }
#endif

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
#ifdef _WIN32
  int n_writables_;
  int n_readables_;
#endif
};

} // anonymous

std::unique_ptr<selector_t> create_select_selector()
{
  return std::make_unique<select_selector_t>();
}

} // cuti

#endif // CUTI_HAS_SELECT_SELECTOR
