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

#include "tcp_connection.hpp"

#include "tcp_acceptor.hpp"

#include <cassert>
#include <iostream>
#include <utility>

namespace xes
{

tcp_connection_t::tcp_connection_t(endpoint_t const& peer)
: socket_(peer.address_family())
, local_endpoint_()
, remote_endpoint_()
{
  socket_.connect(peer);
  local_endpoint_ = socket_.local_endpoint();
  remote_endpoint_ = socket_.remote_endpoint();
}

tcp_connection_t::tcp_connection_t(tcp_socket_t&& socket)
: socket_((assert(!socket.empty()), std::move(socket)))
, local_endpoint_(socket_.local_endpoint())
, remote_endpoint_(socket_.remote_endpoint())
{ }

void tcp_connection_t::set_blocking()
{
  socket_.set_blocking();
}

void tcp_connection_t::set_nonblocking()
{
  socket_.set_nonblocking();
}

char const* tcp_connection_t::write_some(char const* first, char const* last)
{
  return socket_.write_some(first, last);
}

void tcp_connection_t::close_write_end()
{
  socket_.close_write_end();
}
  
char* tcp_connection_t::read_some(char* first, char* last)
{
  return socket_.read_some(first, last);
}

std::ostream& operator<<(std::ostream& os, tcp_connection_t const& connection)
{
  os << connection.local_endpoint() << "<->" << connection.remote_endpoint();
  return os;
}

std::pair<std::unique_ptr<tcp_connection_t>,
          std::unique_ptr<tcp_connection_t>>
make_connected_pair(endpoint_t const& interface)
{
  std::pair<std::unique_ptr<tcp_connection_t>,
            std::unique_ptr<tcp_connection_t>> result;

  tcp_acceptor_t acceptor(interface);
  result.first.reset(new tcp_connection_t(acceptor.local_endpoint()));
  result.second = acceptor.accept();

  return result;
}

std::pair<std::unique_ptr<tcp_connection_t>,
          std::unique_ptr<tcp_connection_t>>
make_connected_pair()
{
  auto interfaces = endpoint_t::local_interfaces(any_port);
  assert(!interfaces.empty());
  return make_connected_pair(interfaces.front());
}

} // xes
