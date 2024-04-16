/*
 * Copyright (C) 2021-2024 CodeShop B.V.
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

#include "nb_inbuf.hpp"

#include "scheduler.hpp"
#include "scoped_guard.hpp"
#include "system_error.hpp"

#include <algorithm>
#include <utility>

namespace cuti
{

nb_inbuf_t::nb_inbuf_t(std::unique_ptr<nb_source_t> source,
                       std::size_t bufsize)
: source_((assert(source != nullptr), std::move(source)))
, checker_(std::nullopt)
, readable_ticket_()
, alarm_ticket_()
, scheduler_(nullptr)
, callback_(nullptr)
, buf_((assert(bufsize != 0), new char[bufsize]))
, rp_(buf_)
, ep_(buf_)
, ebuf_(buf_ + bufsize)
, at_eof_(false)
, error_status_()
{ }

void nb_inbuf_t::enable_throughput_checking(throughput_settings_t settings)
{
  this->disable_throughput_checking();

  checker_.emplace(std::move(settings));
  if(!readable_ticket_.empty())
  {
    assert(alarm_ticket_.empty());
    assert(scheduler_ != nullptr);
    
    auto guard = make_scoped_guard([&] { checker_.reset(); });
    alarm_ticket_ = scheduler_->call_alarm(
      checker_->next_tick(),
      [this](stack_marker_t& base_marker)
      { this->on_next_tick(base_marker); }
    );
    guard.dismiss();
  }
}
  
void nb_inbuf_t::disable_throughput_checking() noexcept
{
  checker_.reset();
  if(!readable_ticket_.empty() && !alarm_ticket_.empty())
  {
    assert(scheduler_ != nullptr);
    scheduler_->cancel(alarm_ticket_);
    alarm_ticket_.clear();
  }
}

char* nb_inbuf_t::read(char *first, char const* last)
{
  assert(this->readable());

  std::size_t count = last - first;
  std::size_t available = ep_ - rp_;
  if(count > available)
  {
    count = available;
  }

  std::copy(rp_, rp_ + count, first);
  rp_ += count;

  return first + count;
}

void nb_inbuf_t::call_when_readable(scheduler_t& scheduler,
                                    callback_t callback)
{
  assert(callback != nullptr);

  this->cancel_when_readable();

  if(this->readable())
  {
    alarm_ticket_ = scheduler.call_alarm(
      duration_t(0),
      [this](stack_marker_t& base_marker)
      { this->on_already_readable(base_marker);
      }
    );
  }
  else
  {
    auto readable_ticket = source_->call_when_readable(
      scheduler,
      [this](stack_marker_t& base_marker)
      { this->on_source_readable(base_marker); }
    );
    if(checker_ != std::nullopt)
    {
      auto guard = make_scoped_guard(
        [&] { scheduler.cancel(readable_ticket); });
      alarm_ticket_ = scheduler.call_alarm(
        checker_->next_tick(),
        [this](stack_marker_t& base_marker)
        { this->on_next_tick(base_marker); }
     );
      guard.dismiss();
    }
    readable_ticket_ = readable_ticket;
  }
        
  scheduler_ = &scheduler;
  callback_ = std::move(callback);
}

void nb_inbuf_t::cancel_when_readable() noexcept
{
  if(!readable_ticket_.empty())
  {
    assert(scheduler_ != nullptr);
    scheduler_->cancel(readable_ticket_);
    readable_ticket_.clear();
  }
  
  if(!alarm_ticket_.empty())
  {
    assert(scheduler_ != nullptr);
    scheduler_->cancel(alarm_ticket_);
    alarm_ticket_.clear();
  }

  scheduler_ = nullptr;
  callback_ = nullptr;
}

nb_inbuf_t::~nb_inbuf_t()
{
  this->cancel_when_readable();
  delete[] buf_;
}

void nb_inbuf_t::on_already_readable(stack_marker_t& base_marker)
{
  assert(readable_ticket_.empty());
  assert(!alarm_ticket_.empty());
  assert(callback_ != nullptr);

  alarm_ticket_.clear();
  scheduler_ = nullptr;
  auto callback = std::move(callback_);
  
  callback(base_marker);
}

void nb_inbuf_t::on_source_readable(stack_marker_t& base_marker)
{
  assert(!this->readable());

  assert(!readable_ticket_.empty());
  assert(scheduler_ != nullptr);
  assert(callback_ != nullptr);
  assert(error_status_ == 0);

  readable_ticket_.clear();

  char* next;
  error_status_ = source_->read(buf_, ebuf_, next);
  if(error_status_ == 0 && checker_ != std::nullopt)
  {
    if(next != nullptr)
    {
      error_status_ = checker_->record_transfer(next - buf_);
    }
    else
    {
      error_status_ = checker_->record_transfer(0);
    }
  }

  if(error_status_ != 0)
  {
    next = buf_;
  }
  else if(next == nullptr)
  {
    // spurious wakeup: reschedule
    auto guard = make_scoped_guard(
      [this] { this->cancel_when_readable(); });
    readable_ticket_ = source_->call_when_readable(
      *scheduler_,
      [this](stack_marker_t& base_marker)
      { this->on_source_readable(base_marker); }
    );
    guard.dismiss();
    return;
  }
    
  // Enter readable state
  if(!alarm_ticket_.empty())
  {
    assert(checker_ != std::nullopt);
    scheduler_->cancel(alarm_ticket_);
    alarm_ticket_.clear();
  }

  rp_ = buf_;
  ep_ = next;
  at_eof_ = rp_ == ep_;

  scheduler_ = nullptr;
  auto callback = std::move(callback_);

  callback(base_marker);
}

void nb_inbuf_t::on_next_tick(stack_marker_t& base_marker)
{
  assert(!this->readable());

  assert(checker_ != std::nullopt);
  assert(!readable_ticket_.empty());
  assert(!alarm_ticket_.empty());
  assert(scheduler_ != nullptr);
  assert(callback_ != nullptr);
  assert(error_status_ == 0);

  alarm_ticket_.clear();

  error_status_ = checker_->record_transfer(0);
  if(error_status_ == 0)
  {
    // schedule next tick
    auto guard = make_scoped_guard(
      [this] { this->cancel_when_readable(); });
    alarm_ticket_ = scheduler_->call_alarm(
      checker_->next_tick(),
      [this](stack_marker_t& base_marker)
      { this->on_next_tick(base_marker); }
    );
    guard.dismiss();
    return;
  }

  // Enter readable state
  scheduler_->cancel(readable_ticket_);
  readable_ticket_.clear();
  
  rp_ = buf_;
  ep_ = buf_;
  at_eof_ = true;

  scheduler_ = nullptr;
  auto callback = std::move(callback_);

  callback(base_marker);
}    
    
} // cuti
