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

#include "nb_outbuf.hpp"

#include "scheduler.hpp"
#include "scoped_guard.hpp"
#include "system_error.hpp"

#include <algorithm>
#include <utility>

namespace cuti
{

nb_outbuf_t::nb_outbuf_t(std::unique_ptr<nb_sink_t> sink,
                         std::size_t bufsize)
: sink_((assert(sink != nullptr), std::move(sink)))
, checker_(std::nullopt)
, writable_ticket_()
, alarm_ticket_()
, scheduler_(nullptr)
, callback_(nullptr)
, buf_((assert(bufsize != 0), new char[bufsize]))
, rp_(buf_)
, wp_(buf_)
, limit_(buf_ + bufsize)
, ebuf_(buf_ + bufsize)
, error_status_()
{ }

char const* nb_outbuf_t::write(char const* first, char const* last)
{
  assert(this->writable());

  std::size_t count = last - first;
  if(error_status_ == 0)
  {
    std::size_t available = limit_ - wp_;
    if(count > available)
    {
      count = available;
    }
    
    std::copy(first, first + count, wp_);
    wp_ += count;
  }

  return first + count;
}

void nb_outbuf_t::enable_throughput_checking(throughput_settings_t settings)
{
  this->disable_throughput_checking();

  checker_.emplace(std::move(settings));
  if(!writable_ticket_.empty())
  {
    assert(alarm_ticket_.empty());
    assert(scheduler_ != nullptr);
    
    auto guard = make_scoped_guard(
      [&] { checker_.reset(); });
    alarm_ticket_ = scheduler_->call_alarm(checker_->next_tick(),
      [this] { this->on_next_tick(); });
    guard.dismiss();
  }
}
  
void nb_outbuf_t::disable_throughput_checking() noexcept
{
  checker_.reset();
  if(!writable_ticket_.empty() && !alarm_ticket_.empty())
  {
    assert(scheduler_ != nullptr);
    scheduler_->cancel(alarm_ticket_);
    alarm_ticket_.clear();
  }
}

void nb_outbuf_t::call_when_writable(scheduler_t& scheduler,
                                     callback_t callback)
{
  assert(callback != nullptr);

  callback_ = nullptr;

  this->cancel_when_writable();

  if(this->writable())
  {
    alarm_ticket_ = scheduler.call_alarm(duration_t(0),
      [this] { this->on_already_writable(); });
  }
  else
  {
    auto writable_ticket = sink_->call_when_writable(scheduler,
      [this] { this->on_sink_writable(); });
    if(checker_ != std::nullopt)
    {
      auto guard = make_scoped_guard(
        [&] { scheduler.cancel(writable_ticket); });
      alarm_ticket_ = scheduler.call_alarm(checker_->next_tick(),
        [this] { this->on_next_tick(); });
      guard.dismiss();
    }
    writable_ticket_ = writable_ticket;
  }

  scheduler_ = &scheduler;
  callback_ = std::move(callback);
}

void nb_outbuf_t::cancel_when_writable() noexcept
{
  if(!writable_ticket_.empty())
  {
    assert(scheduler_ != nullptr);
    scheduler_->cancel(writable_ticket_);
    writable_ticket_.clear();
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

nb_outbuf_t::~nb_outbuf_t()
{
  this->cancel_when_writable();
  delete[] buf_;
}

void nb_outbuf_t::on_already_writable()
{
  assert(writable_ticket_.empty());
  assert(!alarm_ticket_.empty());
  assert(callback_ != nullptr);

  alarm_ticket_.clear();
  scheduler_ = nullptr;
  auto callback = std::move(callback_);

  callback();
}
  
void nb_outbuf_t::on_sink_writable()
{
  assert(!this->writable());

  assert(!writable_ticket_.empty());
  assert(scheduler_ != nullptr);
  assert(callback_ != nullptr);
  assert(error_status_ == 0);

  writable_ticket_.clear();

  char const* next;
  error_status_ = sink_->write(rp_, wp_, next);
  if(error_status_ == 0 && checker_ != std::nullopt)
  {
    if(next != nullptr)
    {
      error_status_ = checker_->record_transfer(next - rp_);
    }
    else
    {
      error_status_ = checker_->record_transfer(0);
    }
  }

  if(error_status_ != 0)
  {
    rp_ = wp_;
  }
  else if(next != nullptr)
  {
    rp_ = next;
  }

  if(rp_ != wp_)
  {
    // more to write: reschedule
    auto guard = make_scoped_guard(
      [this] { this->cancel_when_writable(); });
    writable_ticket_ = sink_->call_when_writable(*scheduler_,
      [this] { this->on_sink_writable(); });
    guard.dismiss();
    return;
  }

  // Enter writable state
  if(!alarm_ticket_.empty())
  {
    assert(checker_ != std::nullopt);
    scheduler_->cancel(alarm_ticket_);
    alarm_ticket_.clear();
  }

  rp_ = buf_;
  wp_ = buf_;
  limit_ = ebuf_;
  
  scheduler_ = nullptr;
  auto callback = std::move(callback_);

  callback();
}

void nb_outbuf_t::on_next_tick()
{
  assert(!this->writable());

  assert(checker_ != std::nullopt);
  assert(!writable_ticket_.empty());
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
      [this] { this->cancel_when_writable(); });
    alarm_ticket_ = scheduler_->call_alarm(checker_->next_tick(),
      [this] { this->on_next_tick(); });
    guard.dismiss();
    return;
  }

  // Enter writable state
  scheduler_->cancel(writable_ticket_);
  writable_ticket_.clear();
    
  rp_ = buf_;
  wp_ = buf_;
  limit_ = ebuf_;
  
  scheduler_ = nullptr;
  auto callback = std::move(callback_);

  callback();
}    

} // cuti
