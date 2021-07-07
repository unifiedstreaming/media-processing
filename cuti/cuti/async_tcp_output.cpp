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

#include "async_tcp_output.hpp"

#include "tcp_connection.hpp"

#include <cassert>
#include <utility>

namespace cuti
{

async_tcp_output_t::async_tcp_output_t(std::shared_ptr<tcp_connection_t> conn)
: conn_((assert(conn != nullptr), std::move(conn)))
, error_status_(0)
, writable_holder_()
{ }

void async_tcp_output_t::call_when_writable(
  scheduler_t& scheduler, callback_t callback)
{
  if(error_status_ != 0)
  {
    writable_holder_.call_alarm(
      scheduler, duration_t::zero(), std::move(callback));
  }
  else
  {
    writable_holder_.call_when_writable(
      scheduler, *conn_, std::move(callback));
  }
}

void async_tcp_output_t::cancel_when_writable() noexcept
{
  writable_holder_.cancel();
}

char const* async_tcp_output_t::write(
  char const* first, char const * last)
{
  char const* next;

  if(error_status_ != 0)
  {
    next = last;
  }
  else
  {
    error_status_ = conn_->write(first, last, next);
  }

  return next;
}

int async_tcp_output_t::error_status() const noexcept
{
  return error_status_;
}

} // cuti
