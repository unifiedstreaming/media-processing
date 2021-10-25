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

#include "nb_inbuf.hpp"

#include "logging_context.hpp"
#include "scheduler.hpp"
#include "scoped_guard.hpp"
#include "system_error.hpp"

#include <algorithm>
#include <utility>

namespace cuti
{

nb_inbuf_t::nb_inbuf_t(logging_context_t& context,
                       std::unique_ptr<nb_source_t> source,
                       std::size_t bufsize)
: context_(context)
, source_((assert(source != nullptr), std::move(source)))
, holder_(*this)
, callback_(nullptr)
, checker_(std::nullopt)
, buf_((assert(bufsize != 0), new char[bufsize]))
, rp_(buf_)
, ep_(buf_)
, ebuf_(buf_ + bufsize)
, at_eof_(false)
, error_status_(0)
{ }

void nb_inbuf_t::enable_throughput_checking(std::size_t min_bytes_per_tick,
                                            unsigned int low_ticks_limit,
                                            duration_t tick_length)
{
  this->disable_throughput_checking();

  checker_.emplace(min_bytes_per_tick, low_ticks_limit, tick_length);
  auto guard = make_scoped_guard([&] { checker_.reset(); });

  if(auto scheduler = holder_.current_scheduler())
  {
    assert(callback_ != nullptr);
    this->call_when_readable(*scheduler, std::move(callback_));
  }

  guard.dismiss();
}
  
void nb_inbuf_t::disable_throughput_checking()
{
  checker_.reset();

  if(auto scheduler = holder_.current_scheduler())
  {
    assert(callback_ != nullptr);
    this->call_when_readable(*scheduler, std::move(callback_));
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

  callback_ = nullptr;

  if(this->readable())
  {
    holder_.call_asap(scheduler);
  }
  else if(checker_ != std::nullopt)
  {
    holder_.call_when_readable(scheduler, *source_, checker_->next_tick());
  }
  else
  {
    holder_.call_when_readable(scheduler, *source_);
  }

  callback_ = std::move(callback);
}

void nb_inbuf_t::cancel_when_readable() noexcept
{
  callback_ = nullptr;
  holder_.cancel();
}

nb_inbuf_t::~nb_inbuf_t()
{
  delete[] buf_;
}

void nb_inbuf_t::check_readable(scheduler_t& scheduler)
{
  assert(callback_ != nullptr);
  callback_t callback = std::move(callback_);

  if(!this->readable())
  {
    char *next;
    error_status_ = source_->read(buf_, ebuf_, next);
    assert(error_status_ == 0 || next == buf_);

    if(error_status_ == 0 && checker_ != std::nullopt)
    {
      error_status_ = checker_->record_transfer(
        next != nullptr ? next - buf_ : 0);
      if(error_status_ != 0)
      {
        if(auto msg = context_.message_at(loglevel_t::warning))
        {
          *msg << "nb_inbuf[" << this->name() <<
            "]: insufficient throughput detected";
        }
        next = buf_;
      }
    }

    if(error_status_ != 0)
    {
      if(auto msg = context_.message_at(loglevel_t::warning))
      {
        *msg << "nb_inbuf[" << this->name() <<
          "]: " << system_error_string(error_status_);
      }
    }
    else if(next == nullptr)
    {
      if(auto msg = context_.message_at(loglevel_t::debug))
      {
        *msg << "nb_inbuf[" << this->name() <<
        "]: can\'t receive yet";
      }
    }
    else
    {
      if(auto msg = context_.message_at(loglevel_t::debug))
      {
        *msg << "nb_inbuf[" << this->name() <<
          "]: " << next - buf_ << " byte(s) received";
      }
    }   
    
    if(next == nullptr)
    {
      // spurious wakeup: reschedule
      this->call_when_readable(scheduler, std::move(callback));
      return;
    }

    rp_ = buf_;
    ep_ = next;
    at_eof_ = rp_ == ep_;
  }
    
  callback();
}

} // cuti
