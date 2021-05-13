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
, last_write_error_(0)
, writing_done_(false)
, last_read_error_(0)
, reading_done_(false)
{
  socket_.connect(peer);
  local_endpoint_ = socket_.local_endpoint();
  remote_endpoint_ = socket_.remote_endpoint();
}

tcp_connection_t::tcp_connection_t(tcp_socket_t&& socket)
: socket_((assert(!socket.empty()), std::move(socket)))
, local_endpoint_(socket_.local_endpoint())
, remote_endpoint_(socket_.remote_endpoint())
, last_write_error_(0)
, writing_done_(false)
, last_read_error_(0)
, reading_done_(false)
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
  char const* result;

  if(writing_done_)
  {
    result = last;
    if(last_write_error_ == 0)
    {
      // write end was closed
#ifdef _WIN32
      last_write_error_ = WSAESHUTDOWN;
#else
      last_write_error_ = EPIPE;
#endif
    }
  }
  else
  {
    last_write_error_ = socket_.write_some(first, last, result);
    writing_done_ = last_write_error_ != 0;
  }

  return result;
}

void tcp_connection_t::close_write_end()
{
  if(!writing_done_)
  {
    last_write_error_ = socket_.close_write_end();
    writing_done_ = true;
  }
}

char* tcp_connection_t::read_some(char* first, char const* last)
{
  char* result;

  if(reading_done_)
  {
    result = first;
  }
  else
  {
    last_read_error_ = socket_.read_some(first, last, result);
    reading_done_ = result == first;
  }

  return result;
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
  auto const& first_local_endpoint = result.first->local_endpoint();

  do
  {
    result.second = acceptor.accept();
    if(result.second != nullptr)
    {
      auto const& second_remote_endpoint = result.second->remote_endpoint();
      if(second_remote_endpoint.address_family() !=
           first_local_endpoint.address_family() ||
         second_remote_endpoint.ip_address() !=
           first_local_endpoint.ip_address() ||
         second_remote_endpoint.port() !=
           first_local_endpoint.port())
      {
        // intruder accepted; disconnect
        result.second.reset();
      }
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
