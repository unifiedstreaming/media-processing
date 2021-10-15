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

#ifndef CUTI_NB_TICKET_HOLDER_HPP
#define CUTI_NB_TICKET_HOLDER_HPP

#include "cancellation_ticket.hpp"
#include "nb_sink.hpp"
#include "nb_source.hpp"
#include "scheduler.hpp"

#include <cassert>

namespace cuti
{

template<typename Target, void (Target::*handler)(scheduler_t&)>
struct nb_ticket_holder_t
{
  explicit nb_ticket_holder_t(Target& target)
  : target_(target)
  , ticket_()
  , scheduler_(nullptr)
  {
    static_assert(handler != nullptr);
  }

  nb_ticket_holder_t(nb_ticket_holder_t const&) = delete;
  nb_ticket_holder_t& operator=(nb_ticket_holder_t const&) = delete;
  
  /*
   * Schedule a call to handler when source is detected to be
   * readable.  Any previously scheduled call is cancelled.
   * The ticket holder remains associated with the scheduler until the
   * handler is called, cancel() is called on the holder, or the
   * holder or destroyed.
   */
  void call_when_readable(scheduler_t& scheduler, nb_source_t& source)
  {
    this->cancel();

    ticket_ = source.call_when_readable(
      scheduler, [this] { this->call_handler(); });
    scheduler_ = &scheduler;
  }

  /*
   * Schedule a call to handler when sink is detected to be writable,
   * canceling any previously scheduled call.
   * The ticket holder remains associated with the scheduler while the
   * call is pending.
   */
  void call_when_writable(scheduler_t& scheduler, nb_sink_t& sink)
  {
    this->cancel();

    ticket_ = sink.call_when_writable(
      scheduler, [this] { this->call_handler(); });
    scheduler_ = &scheduler;
  }

  /*
   * Schedule a call to handler at time point when, canceling any
   * previously scheduled call.
   * The ticket holder remains associated with the scheduler while the
   * call is pending.
   */
  void call_alarm(scheduler_t& scheduler, time_point_t when)
  {
    this->cancel();

    ticket_ = scheduler.call_alarm(when, [this] { this->call_handler(); });
    scheduler_ = &scheduler;
  }

  /*
   * Schedule a call to handler after duration timeout, canceling any
   * previously scheduled call.
   * The ticket holder remains associated with the scheduler while the
   * call is pending.
   */
  void call_alarm(scheduler_t& scheduler, duration_t timeout)
  {
    this->cancel();

    ticket_ = scheduler.call_alarm(timeout, [this] { this->call_handler(); });
    scheduler_ = &scheduler;
  }

  /*
   * Cancels any previously scheduled call; no effect if there is no
   * pending call.
   */
  void cancel() noexcept
  {
    if(!ticket_.empty())
    {
      assert(scheduler_ != nullptr);
      scheduler_->cancel(ticket_);

      ticket_.clear();
      scheduler_ = nullptr;
    }
  }

  /*
   * Destroys the holder, canceling any pending call.
   */
  ~nb_ticket_holder_t()
  {
    this->cancel();
  }

private :
  void call_handler()
  {
    assert(!ticket_.empty());
    ticket_.clear();

    assert(scheduler_ != nullptr);
    scheduler_t& scheduler = *scheduler_;
    scheduler_ = nullptr;

    (target_.*handler)(scheduler);
  }
    
private :
  Target& target_;
  cancellation_ticket_t ticket_;
  scheduler_t* scheduler_;
};

} // cuti

#endif
