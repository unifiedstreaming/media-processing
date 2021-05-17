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

#include "tcp_connection.hpp"

#include "tcp_acceptor.hpp"
#include "resolver.hpp"
#include "system_error.hpp"

#include <cassert>
#include <iostream>
#include <utility>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <errno.h>
#endif

namespace cuti
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

int tcp_connection_t::write(
  char const* first, char const* last, char const*& next)
{
  return socket_.write(first, last, next);
}

int tcp_connection_t::close_write_end()
{
  return socket_.close_write_end();
}

int tcp_connection_t::read(char* first, char const* last, char*& next)
{
  return socket_.read(first, last, next);
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
  auto const& expected_remote = result.first->local_endpoint();

  do
  {
    result.second = acceptor.accept();
    if(result.second != nullptr &&
       result.second->remote_endpoint() != expected_remote)
    {
      // intruder accepted; disconnect
      result.second.reset();
    }
  } while(result.second == nullptr);

  return result;
}

std::pair<std::unique_ptr<tcp_connection_t>,
          std::unique_ptr<tcp_connection_t>>
make_connected_pair()
{
  auto interfaces = local_interfaces(any_port);
  assert(!interfaces.empty());
  return make_connected_pair(interfaces.front());
}

} // cuti
