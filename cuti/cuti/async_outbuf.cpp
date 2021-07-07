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

#include "async_outbuf.hpp"

#include "tcp_connection.hpp"

#include <algorithm>

namespace cuti
{

async_outbuf_t::async_outbuf_t(std::unique_ptr<async_output_t> output)
: async_outbuf_t(std::move(output), default_bufsize)
{ }

async_outbuf_t::async_outbuf_t(
  std::unique_ptr<async_output_t> output,
  std::size_t bufsize)
: output_((assert(output != nullptr), std::move(output)))
, buf_((assert(bufsize != 0), bufsize))
, read_ptr_(buf_.data())
, write_ptr_(buf_.data())
, limit_(buf_.data() + buf_.size())
, writable_now_holder_()
, user_callback_(nullptr)
{ }

int async_outbuf_t::error_status() const noexcept
{
  return output_->error_status();
}

char const* async_outbuf_t::write(char const* first, char const* last)
{
  assert(this->writable());

  auto count = last - first;
  if(count > limit_ - write_ptr_)
  {
    count = limit_ - write_ptr_;
  }

  std::copy(first, first + count, write_ptr_);
  write_ptr_ += count;
  return first + count;
}

void async_outbuf_t::call_when_writable(
  scheduler_t& scheduler, callback_t callback)
{
  assert(callback != nullptr);

  this->cancel_when_writable();

  if(read_ptr_ == write_ptr_ || output_->error_status() != 0)
  {
    read_ptr_ = buf_.data();
    write_ptr_ = buf_.data();
    limit_ = buf_.data() + buf_.size();
  }
  
  if(this->writable())
  {
    writable_now_holder_.call_alarm(scheduler, duration_t::zero(),
      [this] { this->on_writable_now(); });
  }
  else
  {
    output_->call_when_writable(scheduler,
      [this, &scheduler] { this->on_output_writable(scheduler); });
  }
  user_callback_ = std::move(callback);
}

void async_outbuf_t::cancel_when_writable() noexcept
{
  output_->cancel_when_writable();
  writable_now_holder_.cancel();
  user_callback_ = nullptr;
}

async_outbuf_t::~async_outbuf_t()
{
  this->cancel_when_writable();
}

void async_outbuf_t::on_writable_now()
{
  assert(user_callback_ != nullptr);
  callback_t callback = std::move(user_callback_);

  callback();
}

void async_outbuf_t::on_output_writable(scheduler_t& scheduler)
{
  assert(user_callback_ != nullptr);

  char const* next = output_->write(read_ptr_, write_ptr_);
  if(next != write_ptr_)
  {
    // more to flush
    if(next != nullptr)
    {
      // not spurious
      read_ptr_ += next - read_ptr_;
    }
    output_->call_when_writable(scheduler,
      [this, &scheduler] { this->on_output_writable(scheduler); });
  }
  else
  {
    // flushed or error
    read_ptr_ = buf_.data();
    write_ptr_ = buf_.data();
    limit_ = buf_.data() + buf_.size();

    callback_t callback = std::move(user_callback_);
    callback();
  }
}
  
} // cuti
