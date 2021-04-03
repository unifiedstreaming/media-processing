/*
 * Copyright (C) 2021 CodeShop B.V.
 *
 * This file is part of the x264_encoding_service.
 *
 * The x264_encoding_service is free software: you can redistribute it
 * and/or modify it under the terms of version 2 of the GNU General
 * Public License as published by the Free Software Foundation.
 *
 * The x264_encoding_service is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See version 2 of the GNU General Public License for more details.
 *
 * You should have received a copy of version 2 of the GNU General
 * Public License along with the x264_encoding_service.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "tcp_acceptor.hpp"

#include <cassert>
#include <ostream>
#include <utility>

namespace xes
{

tcp_acceptor_t::tcp_acceptor_t(endpoint_t const& endpoint)
: socket_(endpoint.address_family())
, local_endpoint_()
{
  socket_.bind(endpoint);
  socket_.listen();
  local_endpoint_ = socket_.local_endpoint();
}

void tcp_acceptor_t::set_blocking()
{
  socket_.set_blocking();
}

void tcp_acceptor_t::set_nonblocking()
{
  socket_.set_nonblocking();
}

std::unique_ptr<tcp_connection_t> tcp_acceptor_t::accept()
{
  std::unique_ptr<tcp_connection_t> result = nullptr;

  tcp_socket_t accepted = socket_.accept();
  if(!accepted.empty())
  {
    result.reset(new tcp_connection_t(std::move(accepted)));
  }

  return result;
}

std::ostream& operator<<(std::ostream& os, tcp_acceptor_t const& acceptor)
{
  os << acceptor.local_endpoint();
  return os;
}

} // xes
