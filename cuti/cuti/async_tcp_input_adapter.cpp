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

#include "async_tcp_input_adapter.hpp"

#include "tcp_connection.hpp"

#include <cassert>
#include <utility>

namespace cuti
{

async_tcp_input_adapter_t::async_tcp_input_adapter_t(
  std::shared_ptr<tcp_connection_t> conn)
: conn_((assert(conn != nullptr), std::move(conn)))
, readable_holder_()
{ }

void async_tcp_input_adapter_t::call_when_readable(
  scheduler_t& scheduler, callback_t callback)
{
  readable_holder_.call_when_readable(scheduler, *conn_, std::move(callback));
}

void async_tcp_input_adapter_t::cancel_when_readable() noexcept
{
  readable_holder_.cancel();
}

int async_tcp_input_adapter_t::read(
  char* first, char const* last, char*& next)
{
  return conn_->read(first, last, next);
}

} // cuti
