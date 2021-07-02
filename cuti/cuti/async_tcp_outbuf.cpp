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

#include "async_tcp_outbuf.hpp"

#include "tcp_connection.hpp"

namespace cuti
{

async_tcp_outbuf_t::async_tcp_outbuf_t(tcp_connection_t& conn)
: async_tcp_outbuf_t(conn, default_bufsize)
{ }

async_tcp_outbuf_t::async_tcp_outbuf_t(
  tcp_connection_t& conn, std::size_t bufsize)
: async_outbuf_t(bufsize)
, conn_(conn)
{ }

cancellation_ticket_t async_tcp_outbuf_t::do_call_when_writable(
  scheduler_t& scheduler, callback_t callback)
{
  return conn_.call_when_writable(scheduler, std::move(callback));
}

int async_tcp_outbuf_t::do_write(
  char const* first, char const * last, char const *& next)
{
  return conn_.write(first, last, next);
}

} // cuti
