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

#include "ticket_holder.hpp"

#include "tcp_acceptor.hpp"
#include "tcp_connection.hpp"

#include <cassert>
#include <utility>

namespace cuti
{

ticket_holder_t::ticket_holder_t()
: scheduler_callback_([this] { this->on_scheduler_callback(); })
, ticket_()
, scheduler_(nullptr)
, user_callback_(nullptr)
{ }

void ticket_holder_t::call_alarm(scheduler_t& scheduler,
                                 time_point_t when,
                                 callback_t callback)
{
  assert(callback != nullptr);

  this->cancel();

  ticket_ = scheduler.call_alarm(when, scheduler_callback_);
  scheduler_ = &scheduler;
  user_callback_ = std::move(callback);
}

void ticket_holder_t::call_alarm(scheduler_t& scheduler,
                                 duration_t timeout,
                                 callback_t callback)
{
  assert(callback != nullptr);

  this->cancel();

  ticket_ = scheduler.call_alarm(timeout, scheduler_callback_);
  scheduler_ = &scheduler;
  user_callback_ = std::move(callback);
}

void ticket_holder_t::call_when_ready(scheduler_t& scheduler,
                                      tcp_acceptor_t& acceptor,
                                      callback_t callback)
{
  assert(callback != nullptr);

  this->cancel();

  ticket_ = acceptor.call_when_ready(scheduler, scheduler_callback_);
  scheduler_ = &scheduler;
  user_callback_ = std::move(callback);
}

void ticket_holder_t::call_when_writable(scheduler_t& scheduler,
                                         tcp_connection_t& connection,
                                         callback_t callback)
{
  assert(callback != nullptr);

  this->cancel();

  ticket_ = connection.call_when_writable(scheduler, scheduler_callback_);
  scheduler_ = &scheduler;
  user_callback_ = std::move(callback);
}

void ticket_holder_t::call_when_readable(scheduler_t& scheduler,
                                         tcp_connection_t& connection,
                                         callback_t callback)
{
  assert(callback != nullptr);

  this->cancel();

  ticket_ = connection.call_when_readable(scheduler, scheduler_callback_);
  scheduler_ = &scheduler;
  user_callback_ = std::move(callback);
}

void ticket_holder_t::cancel() noexcept
{
  if(!ticket_.empty())
  {
    assert(scheduler_ != nullptr);
    assert(user_callback_ != nullptr);

    scheduler_->cancel(ticket_);

    ticket_.clear();
    scheduler_ = nullptr;
    user_callback_ = nullptr;
  }
}

ticket_holder_t::~ticket_holder_t()
{
  this->cancel();
}

void ticket_holder_t::on_scheduler_callback()
{
  assert(!ticket_.empty());
  assert(scheduler_ != nullptr);
  assert(user_callback_ != nullptr);

  ticket_.clear();
  scheduler_ = nullptr;
  callback_t callback = std::move(user_callback_);

  callback();
}

} // cuti
