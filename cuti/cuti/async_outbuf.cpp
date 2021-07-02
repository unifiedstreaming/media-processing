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

async_outbuf_t::async_outbuf_t(tcp_connection_t& conn)
: async_outbuf_t(conn, default_bufsize)
{ }

async_outbuf_t::async_outbuf_t(tcp_connection_t& conn, std::size_t bufsize)
: conn_(conn)
, buf_((assert(bufsize != 0), new char[bufsize]))
, end_buf_(buf_ + bufsize)
, read_ptr_(buf_)
, write_ptr_(buf_)
, limit_(end_buf_)
, error_status_(0)
, scheduler_(nullptr)
, writable_ticket_()
, callback_(nullptr)
{ }

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

callback_t async_outbuf_t::call_when_writable(
  scheduler_t& scheduler, callback_t callback)
{
  assert(callback != nullptr);

  callback_t result = this->cancel_when_writable();

  if(read_ptr_ == write_ptr_ || error_status_ != 0)
  {
    read_ptr_ = buf_;
    write_ptr_ = buf_;
    limit_ = end_buf_;
  }
  
  if(this->writable())
  {
    writable_ticket_ = scheduler.call_alarm(duration_t::zero(),
      [this] { this->on_writable_now(); });
  }
  else
  {
    writable_ticket_ = conn_.call_when_writable(scheduler,
      [this] { this->on_conn_writable(); });
  }
  scheduler_ = &scheduler;
  callback_ = std::move(callback);

  return result;
}

callback_t async_outbuf_t::cancel_when_writable() noexcept
{
  callback_t result = nullptr;

  if(!writable_ticket_.empty())
  {
    assert(scheduler_ != nullptr);
    assert(callback_ != nullptr);

    scheduler_->cancel(writable_ticket_);
    writable_ticket_.clear();
    
    scheduler_ = nullptr;
    result = std::move(callback_);
  }

  return result;
}

async_outbuf_t::~async_outbuf_t()
{
  this->cancel_when_writable();

  delete[] buf_;
}

void async_outbuf_t::on_writable_now()
{
  assert(scheduler_ != nullptr);
  assert(callback_ != nullptr);

  writable_ticket_.clear();

  scheduler_ = nullptr;
  callback_t callback = std::move(callback_);

  callback();
}

void async_outbuf_t::on_conn_writable()
{
  assert(scheduler_ != nullptr);
  assert(callback_ != nullptr);

  writable_ticket_.clear();

  char const* next;
  int r = conn_.write(read_ptr_, write_ptr_, next);
  if(next != write_ptr_)
  {
    // more to flush
    if(next != nullptr)
    {
      // not spurious
      read_ptr_ += next - read_ptr_;
    }
    writable_ticket_ = conn_.call_when_writable(*scheduler_,
      [this] { this->on_conn_writable(); });
  }
  else
  {
    // flushed or error
    read_ptr_ = buf_;
    write_ptr_ = buf_;
    limit_ = end_buf_;

    error_status_ = r;

    scheduler_ = nullptr;
    callback_t callback = std::move(callback_);
    callback();
  }
}
  
} // cuti
