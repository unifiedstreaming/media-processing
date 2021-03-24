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

#include "endpoint.hpp"

#include "system_error.hpp"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netdb.h>
#endif

namespace xes
{

std::size_t endpoint_size(endpoint_t const& endpoint)
{
  std::size_t result = 0;

  switch(endpoint.sa_family)
  {
  case AF_INET :
    result = sizeof(sockaddr_in);
    break;
  case AF_INET6 :
    result = sizeof(sockaddr_in6);
    break;
  default :
    throw system_exception_t("Address family " +
      std::to_string(endpoint.sa_family) + " not supported");
    break;
  }

  return result;
}

std::string ip_address(endpoint_t const& endpoint)
{
  // 0000:0000:0000:0000:0000:0000:255.255.255.255 (+ terminating '\0')
  char result[46]; 

  int r = getnameinfo(
    &endpoint, static_cast<socklen_t>(endpoint_size(endpoint)),
    result, static_cast<socklen_t>(sizeof result),
    nullptr, 0,
    NI_NUMERICHOST);

  if(r != 0)
  {
#ifdef _WIN32
    int cause = last_system_error();
    throw system_exception_t("Can't determine IP address", cause);
#else
    throw system_exception_t(
      std::string("Can't determine IP address: ") + gai_strerror(r));
#endif
  }

  return result;
}

unsigned int port_number(endpoint_t const& endpoint)
{
  unsigned int result = 0;

  switch(endpoint.sa_family)
  {
  case AF_INET :
    result =
      htons(reinterpret_cast<const sockaddr_in *>(&endpoint)->sin_port);
    break;
  case AF_INET6 :
    result =
      htons(reinterpret_cast<const sockaddr_in6 *>(&endpoint)->sin6_port);
    break;
  default :
    throw system_exception_t("Address family " +
      std::to_string(endpoint.sa_family) + " not supported");
    break;
  }

  return result;
}

} // namespace xes
