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

#include "scheduler.hpp"

#include <algorithm>
#include <utility>

namespace cuti
{

nb_inbuf_t::nb_inbuf_t(std::unique_ptr<nb_source_t> source,
                       std::size_t bufsize)
: source_((assert(source != nullptr), std::move(source)))
, buf_((assert(bufsize != 0), bufsize))
, rp_(buf_.data())
, ep_(buf_.data())
, at_eof_(false)
, error_status_(0)
, holder_(*this)
, callback_(nullptr)
{ }

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
    holder_.call_alarm(scheduler, duration_t::zero());
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

void nb_inbuf_t::on_readable(scheduler_t& scheduler)
{
  assert(callback_ != nullptr);
  callback_t callback = std::move(callback_);

  if(!this->readable())
  {
    char *next;
    error_status_ = source_->read(
      buf_.data(), buf_.data() + buf_.size(), next);

    if(error_status_ != 0)
    {
      next = buf_.data();
    }
    else if(next == nullptr)
    {
      // spurious wakeup: reschedule
      holder_.call_when_readable(scheduler, *source_);
      callback_ = std::move(callback);
      return;
    }

    rp_ = buf_.data();
    ep_ = next;
    at_eof_ = next == buf_.data();
  }

  callback();
}

} // cuti
