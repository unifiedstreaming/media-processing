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

#ifndef CUTI_NB_TICKETS_HOLDER_HPP
#define CUTI_NB_TICKETS_HOLDER_HPP

#include "cancellation_ticket.hpp"
#include "nb_sink.hpp"
#include "nb_source.hpp"
#include "scheduler.hpp"
#include "scoped_guard.hpp"

#include <cassert>

namespace cuti
{

template<typename Target, void (Target::*handler)(scheduler_t&)>
struct nb_tickets_holder_t
{
  explicit nb_tickets_holder_t(Target& target)
  : target_(target)
  , alarm_ticket_()
  , io_ticket_()
  , scheduler_(nullptr)
  {
    static_assert(handler != nullptr);
  }

  nb_tickets_holder_t(nb_tickets_holder_t const&) = delete;
  nb_tickets_holder_t& operator=(nb_tickets_holder_t const&) = delete;

  /*
   * Returns a pointer to the currently associated scheduler if a
   * handler call is pending, nullptr otherwise.
   */
  scheduler_t* current_scheduler() const
  {
    return scheduler_;
  }

  /*
   * Call handler as soon as possible, canceling any previously
   * scheduled call.
   * The holder remains associated with the scheduler while the call
   * is pending.
   */
  void call_asap(scheduler_t& scheduler)
  {
    this->cancel();

    alarm_ticket_ = scheduler.call_alarm(
      duration_t::zero(), [this] { this->on_alarm(); });
    scheduler_ = &scheduler;
  }
      
  /*
   * Call handler when source is detected to be readable, canceling
   * any previously scheduled call.
   * The holder remains associated with the scheduler while the call
   * is pending.
   */
  void call_when_readable(scheduler_t& scheduler,
                          nb_source_t& source)
  {
    this->cancel();

    io_ticket_ = source.call_when_readable(
      scheduler, [this] { this->on_io(); });
    scheduler_ = &scheduler;
  }

  /*
   * Call handler when source is detected to be readable or timeout is
   * reached (whichever happens first), canceling any previously
   * scheduled call.
   * The holder remains associated with the scheduler while the call
   * is pending.
   */
  void call_when_readable(scheduler_t& scheduler,
                          nb_source_t& source,
                          time_point_t timeout)
  {
    this->cancel();

    auto alarm_ticket = scheduler.call_alarm(
      timeout, [this] { this->on_alarm(); });

    auto guard = make_scoped_guard([&] { scheduler.cancel(alarm_ticket); });
    auto io_ticket = source.call_when_readable(
      scheduler, [this] { this->on_io(); });
    guard.dismiss();

    alarm_ticket_ = alarm_ticket;
    io_ticket_ = io_ticket;
    scheduler_ = &scheduler;
  }

  /*
   * Call handler when sink is detected to be writable, canceling any
   * previously scheduled call.
   * The holder remains associated with the scheduler while the
   * call is pending.
   */
  void call_when_writable(scheduler_t& scheduler,
                          nb_sink_t& sink)
  {
    this->cancel();

    io_ticket_ = sink.call_when_writable(
      scheduler, [this] { this->on_io(); });
    scheduler_ = &scheduler;
  }

  /*
   * Call handler when sink is detected to be writable or timeout is
   * reached (whichever happens first), canceling any previously
   * scheduled call.
   * The holder remains associated with the scheduler while the call
   * is pending.
   */
  void call_when_writable(scheduler_t& scheduler,
                          nb_sink_t& sink,
                          time_point_t timeout)
  {
    this->cancel();

    auto alarm_ticket = scheduler.call_alarm(
      timeout, [this] { this->on_alarm(); });

    auto guard = make_scoped_guard([&] { scheduler.cancel(alarm_ticket); });
    auto io_ticket = sink.call_when_writable(
      scheduler, [this] { this->on_io(); });
    guard.dismiss();

    alarm_ticket_ = alarm_ticket;
    io_ticket_ = io_ticket;
    scheduler_ = &scheduler;
  }

  /*
   * Cancels any previously scheduled call; no effect if there is no
   * pending call.
   */
  void cancel() noexcept
  {
    if(scheduler_ != nullptr)
    {
      if(!io_ticket_.empty())
      {
        scheduler_->cancel(io_ticket_);
        io_ticket_.clear();
      }

      if(!alarm_ticket_.empty())
      {
        scheduler_->cancel(alarm_ticket_);
        alarm_ticket_.clear();
      }

      scheduler_ = nullptr;
    }
  }

  /*
   * Destroys the holder, canceling any pending call.
   */
  ~nb_tickets_holder_t()
  {
    this->cancel();
  }

private :
  void on_alarm()
  {
    assert(scheduler_ != nullptr);
    scheduler_t& scheduler = *scheduler_;
    scheduler_ = nullptr;

    if(!io_ticket_.empty())
    {
      scheduler.cancel(io_ticket_);
      io_ticket_.clear();
    }

    assert(!alarm_ticket_.empty());
    alarm_ticket_.clear();

    (target_.*handler)(scheduler);
  }    

  void on_io()
  {
    assert(scheduler_ != nullptr);
    scheduler_t& scheduler = *scheduler_;
    scheduler_ = nullptr;

    assert(!io_ticket_.empty());
    io_ticket_.clear();

    if(!alarm_ticket_.empty())
    {
      scheduler.cancel(alarm_ticket_);
      alarm_ticket_.clear();
    }

    (target_.*handler)(scheduler);
  }
    
private :
  Target& target_;
  cancellation_ticket_t alarm_ticket_;
  cancellation_ticket_t io_ticket_;
  scheduler_t* scheduler_;
};

} // cuti

#endif
