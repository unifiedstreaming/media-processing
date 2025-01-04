/*
 * Copyright (C) 2021-2025 CodeShop B.V.
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

#include "tcp_acceptor.hpp"

#include <cassert>
#include <ostream>
#include <utility>

namespace cuti
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

int tcp_acceptor_t::accept(std::unique_ptr<tcp_connection_t>& accepted)
{
  tcp_socket_t incoming;
  int result = socket_.accept(incoming);

  if(incoming.empty())
  {
    accepted.reset();
  }
  else
  {
    accepted.reset(new tcp_connection_t(std::move(incoming)));
  }

  return result;
}

std::ostream& operator<<(std::ostream& os, tcp_acceptor_t const& acceptor)
{
  os << acceptor.local_endpoint();
  return os;
}

} // cuti
