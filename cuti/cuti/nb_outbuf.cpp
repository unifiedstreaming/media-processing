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
, already_writable_holder_(*this)
, sink_writable_holder_(*this)
, callback_(nullptr)
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
  std::size_t available = limit_ - wp_;
  if(count > available)
  {
    count = available;
  }

  std::copy(first, first + count, wp_);
  wp_ += count;

  return first + count;
}

void nb_outbuf_t::call_when_writable(scheduler_t& scheduler,
                                     callback_t callback)
{
  assert(callback != nullptr);

  this->cancel_when_writable();

  if(this->writable())
  {
    already_writable_holder_.call_alarm(scheduler, duration_t::zero());
  }
  else if(rp_ == wp_ || error_status_ != 0)
  {
    rp_ = buf_;
    wp_ = buf_;
    limit_ = ebuf_;

    already_writable_holder_.call_alarm(scheduler, duration_t::zero());
  }
  else
  {
    sink_writable_holder_.call_when_writable(scheduler, *sink_);
  }
  
  callback_ = std::move(callback);
}

void nb_outbuf_t::cancel_when_writable() noexcept
{
  callback_ = nullptr;
  sink_writable_holder_.cancel();
  already_writable_holder_.cancel();
}

nb_outbuf_t::~nb_outbuf_t()
{
  delete[] buf_;
}

void nb_outbuf_t::on_already_writable(scheduler_t& scheduler)
{
  assert(callback_ != nullptr);
  callback_t callback = std::move(callback_);

  if(!this->writable())
  {
    // transitioned to non-writable since last call_when_writable()
    this->call_when_writable(scheduler, std::move(callback));
    return;
  }

  callback();
}
    
void nb_outbuf_t::on_sink_writable(scheduler_t& scheduler)
{
  assert(callback_ != nullptr);
  callback_t callback = std::move(callback_);

  assert(!this->writable());
  assert(rp_ != wp_);
  assert(error_status_ == 0);

  char const* next;
  error_status_ = sink_->write(rp_, wp_, next);
  assert(error_status_ == 0 || next == wp_);

  if(error_status_ != 0)
  {
    if(auto msg = context_.message_at(loglevel_t::error))
    {
      *msg << "nb_outbuf[" << this->name() <<
        "]: " << system_error_string(error_status_);
    }
  }
  else if(next == nullptr)
  {
    if(auto msg = context_.message_at(loglevel_t::debug))
    {
      *msg << "nb_outbuf[" << this->name() <<
        "]: can\'t send yet";
    }
  }
  else
  {
    if(auto msg = context_.message_at(loglevel_t::debug))
    {
      *msg << "nb_outbuf[" << this->name() <<
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
    this->call_when_writable(scheduler, std::move(callback));
    return;
  }
      
  rp_ = buf_;
  wp_ = buf_;
  limit_ = ebuf_;    

  callback();
}

} // cuti
