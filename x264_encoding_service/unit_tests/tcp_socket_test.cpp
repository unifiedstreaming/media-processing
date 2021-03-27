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

#include "tcp_socket.hpp"
#include "endpoint_list.hpp"

#include <utility>

// Enable assert()
#undef NDEBUG
#include <cassert>

namespace // anonymous
{

void socket_state_for_family(int family)
{
  xes::tcp_socket_t sock0;
  assert(sock0.empty());

  xes::tcp_socket_t sock1(family);
  assert(!sock1.empty());

  xes::tcp_socket_t sock2 = std::move(sock1);
  assert(sock1.empty());
  assert(!sock2.empty());

  swap(sock1, sock2);
  assert(!sock1.empty());
  assert(sock2.empty());

  sock2 = std::move(sock1);
  assert(sock1.empty());
  assert(!sock2.empty());

  sock1 = std::move(sock1);
  assert(sock1.empty());

  sock2 = std::move(sock2);
  assert(!sock2.empty());

  sock2 = std::move(sock1);
  assert(sock1.empty());
  assert(sock2.empty());
}
  
void socket_state()
{
  xes::endpoint_list_t endpoints(xes::local_interfaces, xes::any_port);
  for(auto const& endpoint : endpoints)
  {
    socket_state_for_family(xes::endpoint_family(endpoint));
  }
}

} // anonymous

int main()
{
  socket_state();

  return 0;
}
