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

#include "nb_outbuf.hpp"

#include "logging_context.hpp"
#include "scheduler.hpp"
#include "system_error.hpp"

#include <algorithm>
#include <utility>

namespace cuti
{

nb_outbuf_t::nb_outbuf_t(logging_context_t& context,
                         std::unique_ptr<nb_sink_t> sink,
                         std::size_t bufsize)
: context_(context)
, sink_((assert(sink != nullptr), std::move(sink)))
, holder_(*this)
, callback_(nullptr)
, checker_(std::nullopt)
, buf_((assert(bufsize != 0), new char[bufsize]))
, rp_(buf_)
, wp_(buf_)
, limit_(buf_ + bufsize)
, ebuf_(buf_ + bufsize)
, error_status_(0)
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

void nb_outbuf_t::enable_throughput_checking(std::size_t min_bytes_per_tick,
                                             unsigned int low_ticks_limit,
                                             duration_t tick_length)
{
  auto guard = make_scoped_guard([this]
  {
    checker_.reset();
    callback_ = nullptr;
    holder_.cancel();
  });

  checker_.emplace(min_bytes_per_tick, low_ticks_limit, tick_length);

  scheduler_t* scheduler = holder_.current_scheduler();
  if(scheduler != nullptr && !this->writable())
  {
    // Schedule timeout on next tick
    assert(callback_ != nullptr);
    holder_.call_when_writable(*scheduler, *sink_, checker_->next_tick());
  }

  guard.dismiss();
}
  
void nb_outbuf_t::disable_throughput_checking()
{
  checker_.reset();
  auto guard = make_scoped_guard([this]
  {
    callback_ = nullptr;
    holder_.cancel();
  });

  scheduler_t* scheduler = holder_.current_scheduler();
  if(scheduler != nullptr && !this->writable())
  {
    // Cancel timeout on next tick
    assert(callback_ != nullptr);
    holder_.call_when_writable(*scheduler, *sink_);
  }

  guard.dismiss();
}

void nb_outbuf_t::call_when_writable(scheduler_t& scheduler,
                                     callback_t callback)
{
  assert(callback != nullptr);

  callback_ = nullptr;

  if(this->writable())
  {
    holder_.call_asap(scheduler);
  }
  else if(checker_ != std::nullopt)
  {
    holder_.call_when_writable(scheduler, *sink_, checker_->next_tick());
  }
  else
  {
    holder_.call_when_writable(scheduler, *sink_);
  }
  
  callback_ = std::move(callback);
}

void nb_outbuf_t::cancel_when_writable() noexcept
{
  callback_ = nullptr;
  holder_.cancel();
}

nb_outbuf_t::~nb_outbuf_t()
{
  delete[] buf_;
}

void nb_outbuf_t::check_writable(scheduler_t& scheduler)
{
  assert(callback_ != nullptr);
  callback_t callback = std::move(callback_);

  if(!this->writable())
  {
    assert(rp_ != wp_);
    assert(error_status_ == 0);

    char const* next;
    error_status_ = sink_->write(rp_, wp_, next);
    assert(error_status_ == 0 || next == wp_);

    if(error_status_ == 0 && checker_ != std::nullopt)
    {
      error_status_ = checker_->record_transfer(
        next != nullptr ? next - rp_ : 0);
      if(error_status_ != 0)
      {
        if(auto msg = context_.message_at(loglevel_t::warning))
        {
          *msg << "nb_outbuf[" << *this <<
            "]: insufficient throughput detected";
        }
        next = wp_;
      }
    }

    if(error_status_ != 0)
    {
      if(auto msg = context_.message_at(loglevel_t::warning))
      {
        *msg << "nb_outbuf[" << *this <<
          "]: " << system_error_string(error_status_);
      }
    }
    else if(next == nullptr)
    {
      if(auto msg = context_.message_at(loglevel_t::debug))
      {
        *msg << "nb_outbuf[" << *this <<
          "]: can\'t send yet";
      }
    }
    else
    {
      if(auto msg = context_.message_at(loglevel_t::debug))
      {
        *msg << "nb_outbuf[" << *this <<
          "]: " << next - rp_ << " byte(s) sent";
      }
    }   
    
    if(next != nullptr)
    {
      rp_ = next;
    }

    if(rp_ != wp_)
    {
      // more to write: reschedule
      if(checker_ != std::nullopt)
      {
        holder_.call_when_writable(scheduler, *sink_, checker_->next_tick());
      }
      else
      {
        holder_.call_when_writable(scheduler, *sink_);
      }
      callback_ = std::move(callback);
      return;
    }
      
    rp_ = buf_;
    wp_ = buf_;
    limit_ = ebuf_;
  }

  callback();
}

} // cuti
