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

#include "scheduler.hpp"

#include <algorithm>
#include <utility>

namespace cuti
{

nb_outbuf_t::nb_outbuf_t(std::unique_ptr<nb_sink_t> sink,
                         std::size_t bufsize)
: sink_((assert(sink != nullptr), std::move(sink)))
, buf_((assert(bufsize != 0), bufsize))
, rp_(buf_.data())
, wp_(buf_.data())
, limit_(buf_.data() + buf_.size())
, error_status_(0)
, holder_(*this)
, callback_(nullptr)
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

  callback_ = nullptr;

  if(this->writable() || error_status_ != 0)
  {
    holder_.call_alarm(scheduler, duration_t::zero());
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

void nb_outbuf_t::on_writable(scheduler_t& scheduler)
{
  assert(callback_ != nullptr);
  callback_t callback = std::move(callback_);

  if(!this->writable())
  {
    if(error_status_ == 0 && rp_ != wp_)
    {
      char const* next;
      error_status_ = sink_->write(rp_, wp_, next);
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
        holder_.call_when_writable(scheduler, *sink_);
        callback_ = std::move(callback);
        return;
      }
    }

    rp_ = buf_.data();
    wp_ = buf_.data();
    limit_ = buf_.data() + buf_.size();    
  }

  callback();
}

} // cuti
