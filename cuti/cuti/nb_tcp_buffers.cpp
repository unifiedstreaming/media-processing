/*
 * Copyright (C) 2021-2023 CodeShop B.V.
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

#include "nb_tcp_buffers.hpp"

#include "scheduler.hpp"

#include <sstream>

namespace cuti
{

namespace // anonymous
{

struct nb_tcp_source_t : nb_source_t
{
  explicit nb_tcp_source_t(std::shared_ptr<tcp_connection_t> conn)
  : conn_((assert(conn != nullptr), std::move(conn)))
  { }

  int read(char* first, char const* last, char*& next) override
  {
    return conn_->read(first, last, next);
  }

  cancellation_ticket_t
  call_when_readable(scheduler_t& scheduler, callback_t callback) override
  {
    return conn_->call_when_readable(scheduler, std::move(callback));
  }

  void print(std::ostream& os) const override
  {
    os << *conn_;
  }

private :
  std::shared_ptr<tcp_connection_t> conn_;
};

struct nb_tcp_sink_t : nb_sink_t
{
  explicit nb_tcp_sink_t(std::shared_ptr<tcp_connection_t> conn)
  : conn_((assert(conn != nullptr), std::move(conn)))
  { }

  int write(char const* first, char const* last, char const*& next) override
  {
    return conn_->write(first, last, next);
  }

  cancellation_ticket_t
  call_when_writable(scheduler_t& scheduler, callback_t callback) override
  {
    return conn_->call_when_writable(scheduler, std::move(callback));
  }

  void print(std::ostream& os) const override
  {
    os << *conn_;
  }

  ~nb_tcp_sink_t() override
  {
    conn_->close_write_end();
  }

private :
  std::shared_ptr<tcp_connection_t> conn_;
};
   
} // anonymous

std::pair<std::unique_ptr<nb_inbuf_t>, std::unique_ptr<nb_outbuf_t>>
make_nb_tcp_buffers(std::unique_ptr<tcp_connection_t> conn,
                    std::size_t inbufsize,
                    std::size_t outbufsize)
{
  assert(conn != nullptr);

  conn->set_nonblocking();

  auto shared_conn = std::shared_ptr<tcp_connection_t>(std::move(conn));

  auto source = std::make_unique<nb_tcp_source_t>(shared_conn);
  auto sink = std::make_unique<nb_tcp_sink_t>(std::move(shared_conn));

  return std::make_pair(
    std::make_unique<nb_inbuf_t>(std::move(source), inbufsize),
    std::make_unique<nb_outbuf_t>(std::move(sink), outbufsize));
}

} // cuti
