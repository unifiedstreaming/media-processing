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

#include "endpoint_storage.hpp"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netdb.h>
#endif

// enable assert()
#undef NDEBUG
#include <cassert>

namespace // anonymous
{

void ipv4_storage()
{
  xes::endpoint_storage_t storage(AF_INET);

  assert(xes::endpoint_family(*storage) == AF_INET);
  assert(xes::endpoint_size(*storage) == sizeof(sockaddr_in));
  assert(xes::ip_address(*storage) == "0.0.0.0");
  assert(xes::port_number(*storage) == 0);
}

void ipv6_storage()
{
  xes::endpoint_storage_t storage(AF_INET6);

  assert(xes::endpoint_family(*storage) == AF_INET6);
  assert(xes::endpoint_size(*storage) == sizeof(sockaddr_in6));
  assert(xes::ip_address(*storage) == "::");
  assert(xes::port_number(*storage) == 0);
}

} // anonymous

int main()
{
  ipv4_storage();
  ipv6_storage();

  return 0;
}
