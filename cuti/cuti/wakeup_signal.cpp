/*
 * Copyright (C) 2022 CodeShop B.V.
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

#include "wakeup_signal.hpp"

#include <cassert>
#include <tuple>
#include <utility>

namespace cuti
{

wakeup_signal_t::wakeup_signal_t()
: mutex_()
, activation_count_(0)
, write_end_(nullptr)
, read_end_(nullptr)
{
  std::tie(write_end_, read_end_) = make_connected_pair();
}

bool wakeup_signal_t::active() const noexcept
{
  std::scoped_lock<std::mutex> lock(mutex_);

  return activation_count_ != 0;
}

void wakeup_signal_t::activate() noexcept
{
  std::scoped_lock<std::mutex> lock(mutex_);

  if(activation_count_ == 0)
  {
    char data[1];
    data[0] = '*';
    char const* next;
    write_end_->write(data, data + 1, next);
    assert(next == data + 1);
  }

  ++activation_count_;
}
    
void wakeup_signal_t::deactivate() noexcept
{
  std::scoped_lock<std::mutex> lock(mutex_);

  --activation_count_;

  if(activation_count_ == 0)
  {
    char data[1];
    char* next;
    read_end_->read(data, data + 1, next);
    assert(next == data + 1);
    assert(data[0] == '*');
  }
}
  
cancellation_ticket_t wakeup_signal_t::call_when_active(
  scheduler_t& scheduler, callback_t callback) const
{
  return read_end_->call_when_readable(scheduler, std::move(callback));
}

wakeup_signal_watcher_t::wakeup_signal_watcher_t(
  wakeup_signal_t const& wakeup_signal, scheduler_t& scheduler)
: wakeup_signal_(wakeup_signal)
, scheduler_(scheduler)
, ticket_()
, callback_(nullptr)
{ }

void wakeup_signal_watcher_t::call_when_active(callback_t callback)
{
  assert(callback != nullptr);

  this->cancel_when_active();

  ticket_ = wakeup_signal_.call_when_active(
    scheduler_, [this] { this->on_scheduler_callback(); });
  callback_ = std::move(callback);
}

void wakeup_signal_watcher_t::cancel_when_active() noexcept
{
  if(!ticket_.empty())
  {
    scheduler_.cancel(ticket_);
    callback_ = nullptr;
    ticket_.clear();
  }
}

void wakeup_signal_watcher_t::on_scheduler_callback()
{
  assert(!ticket_.empty());
  ticket_.clear();
  
  assert(callback_ != nullptr);
  callback_t callback = std::move(callback_);
  callback_ = nullptr;

  if(!wakeup_signal_.active())
  {
    // spurious wakeup: reschedule callback
    this->call_when_active(std::move(callback));
    return;
  }

  callback();
}

wakeup_signal_watcher_t::~wakeup_signal_watcher_t()
{
  this->cancel_when_active();
}

} // cuti
