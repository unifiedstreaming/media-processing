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
, writable_ticket_()
, scheduler_(nullptr)
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

  this->cancel_when_writable();

  if(this->writable() || error_status_ != 0)
  {
    writable_ticket_ = scheduler.call_alarm(
      duration_t::zero(), [this] { this->on_writable(); });
  }
  else
  {
    writable_ticket_ = sink_->call_when_writable(
      scheduler, [this] { this->on_writable(); });
  }

  scheduler_ = &scheduler;
  callback_ = std::move(callback);
}

void nb_outbuf_t::cancel_when_writable() noexcept
{
  if(!writable_ticket_.empty())
  {
    assert(scheduler_ != nullptr);
    assert(callback_ != nullptr);
    scheduler_->cancel(writable_ticket_);

    writable_ticket_.clear();
    scheduler_ = nullptr;
    callback_ = nullptr;
  }
}

nb_outbuf_t::~nb_outbuf_t()
{
  this->cancel_when_writable();
}

void nb_outbuf_t::on_writable()
{
  assert(!writable_ticket_.empty());
  writable_ticket_.clear();

  assert(scheduler_ != nullptr);
  scheduler_t& scheduler = *scheduler_;
  scheduler_ = nullptr;

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
        writable_ticket_ = sink_->call_when_writable(
          scheduler, [this] { this->on_writable(); });
        scheduler_ = &scheduler;
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
