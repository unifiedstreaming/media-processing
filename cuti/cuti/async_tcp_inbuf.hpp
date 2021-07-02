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

#ifndef CUTI_TCP_INBUF_HPP_
#define CUTI_TCP_INBUF_HPP_

#include "async_inbuf.hpp"

namespace cuti
{

struct tcp_connection_t;

/*
 * Asynchronous tcp input buffer.
 */
struct CUTI_ABI async_tcp_inbuf_t : async_inbuf_t
{
  static size_t constexpr default_bufsize = 256 * 1024;

  /*
   * Construct an asynchronous input buffer for conn, using the
   * default buffer size. The connection must stay alive until *this
   * is destroyed.
   */
  explicit async_tcp_inbuf_t(tcp_connection_t& conn);

  /*
   * Construct an asynchronous input buffer for conn, using the
   * specified buffer size. The connection must stay alive until *this
   * is destroyed.
   */
  async_tcp_inbuf_t(tcp_connection_t& conn, std::size_t bufsize);

private :
  cancellation_ticket_t
  do_call_when_readable(scheduler_t& scheduler, callback_t callback) override;

  int
  do_read(char* first, char const* last, char*& next) override;

private :
  tcp_connection_t& conn_;
};

} // cuti

#endif
