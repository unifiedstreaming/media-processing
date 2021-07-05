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
, buf_((assert(bufsize != 0), new char[bufsize]))
, end_buf_(buf_ + bufsize)
, read_ptr_(buf_)
, limit_(buf_)
, eof_seen_(false)
, error_status_(0)
, scheduler_(nullptr)
, readable_ticket_()
, callback_(nullptr)
{ }

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
    
callback_t async_inbuf_t::call_when_readable(
  scheduler_t& scheduler, callback_t callback)
{
  assert(callback != nullptr);

  callback_t result = cancel_when_readable();

  if(this->readable())
  {
    readable_ticket_ = scheduler.call_alarm(duration_t::zero(),
      [this] { this->on_readable_now(); });
  }
  else
  {
    readable_ticket_ = adapter_->call_when_readable(scheduler,
      [this] { this->on_adapter_readable(); });
  }
  scheduler_ = &scheduler;
  callback_ = std::move(callback);

  return result;
};
    
callback_t async_inbuf_t::cancel_when_readable() noexcept
{
  callback_t result = nullptr;

  if(!readable_ticket_.empty())
  {
    assert(scheduler_ != nullptr);
    assert(callback_ != nullptr);
    
    scheduler_->cancel(readable_ticket_);
    readable_ticket_.clear();
    
    scheduler_ = nullptr;
    result = std::move(callback_);
  }

  return result;
}

async_inbuf_t::~async_inbuf_t()
{
  this->cancel_when_readable();

  delete[] buf_;
}

void async_inbuf_t::on_readable_now()
{
  assert(scheduler_ != nullptr);
  assert(callback_ != nullptr);

  readable_ticket_.clear();

  scheduler_ = nullptr;
  callback_t callback = std::move(callback_);

  callback();
}

void async_inbuf_t::on_adapter_readable()
{
  assert(scheduler_ != nullptr);
  assert(callback_ != nullptr);

  readable_ticket_.clear();
  
  char* next;
  int r = adapter_->read(buf_, end_buf_, next);
  if(next == nullptr)
  {
    // spurious wakeup: try again
    readable_ticket_ = adapter_->call_when_readable(*scheduler_,
      [this] { this->on_adapter_readable(); });
  }
  else
  {
    // got data, eof, or error
    read_ptr_ = buf_;
    limit_ = next;
    eof_seen_ = next == buf_;
    error_status_ = r;
      
    scheduler_ = nullptr;
    callback_t callback = std::move(callback_);
    callback();
  }
}
    
} // cuti
