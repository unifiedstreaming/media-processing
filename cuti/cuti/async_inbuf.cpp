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

#include "async_inbuf.hpp"

#include <algorithm>

namespace cuti
{

async_inbuf_t::async_inbuf_t(std::unique_ptr<async_input_adapter_t> adapter)
: async_inbuf_t(std::move(adapter), default_bufsize)
{ }

async_inbuf_t::async_inbuf_t(
  std::unique_ptr<async_input_adapter_t> adapter,
  std::size_t bufsize)
: adapter_((assert(adapter != nullptr), std::move(adapter)))
, buf_((assert(bufsize != 0), bufsize))
, read_ptr_(buf_.data())
, limit_(buf_.data())
, eof_seen_(false)
, readable_now_holder_()
, user_callback_(nullptr)
{ }

int async_inbuf_t::error_status() const noexcept
{
  return adapter_->error_status();
}

char* async_inbuf_t::read(char* first, char const* last)
{
  assert(this->readable());

  auto count = last - first;
  if(count > limit_ - read_ptr_)
  {
    count = limit_ - read_ptr_;
  }

  std::copy(read_ptr_, read_ptr_ + count, first);
  read_ptr_ += count;
  return first + count;
}
    
void async_inbuf_t::call_when_readable(scheduler_t& scheduler,
                                       callback_t callback)
{
  assert(callback != nullptr);

  this->cancel_when_readable();

  if(this->readable())
  {
    readable_now_holder_.call_alarm(scheduler, duration_t::zero(),
      [this] { this->on_readable_now(); });
  }
  else
  {
    adapter_->call_when_readable(scheduler,
      [this, &scheduler] { this->on_adapter_readable(scheduler); });
  }
  user_callback_ = std::move(callback);
};
    
void async_inbuf_t::cancel_when_readable() noexcept
{
  adapter_->cancel_when_readable();
  readable_now_holder_.cancel();
  user_callback_ = nullptr;
}

async_inbuf_t::~async_inbuf_t()
{
  this->cancel_when_readable();
}

void async_inbuf_t::on_readable_now()
{
  assert(user_callback_ != nullptr);
  callback_t callback = std::move(user_callback_);

  callback();
}

void async_inbuf_t::on_adapter_readable(scheduler_t& scheduler)
{
  assert(user_callback_ != nullptr);

  char* next = adapter_->read(buf_.data(), buf_.data() + buf_.size());
  if(next == nullptr)
  {
    // spurious wakeup: try again
    adapter_->call_when_readable(scheduler,
      [this, &scheduler] { this->on_adapter_readable(scheduler); });
  }
  else
  {
    // got data, eof, or error
    read_ptr_ = buf_.data();
    limit_ = next;
    eof_seen_ = next == buf_.data();
      
    callback_t callback = std::move(user_callback_);
    callback();
  }
}
    
} // cuti