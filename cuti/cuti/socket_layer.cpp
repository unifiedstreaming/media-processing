/*
 * Copyright (C) 2025-2026 CodeShop B.V.
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

#include "socket_layer.hpp"
#include "system_error.hpp"

#include <cassert>

#ifdef _WIN32

#include <winsock2.h>
#include <ws2tcpip.h>

#else // POSIX

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#endif

namespace cuti
{

struct socket_layer_t::initializer_t
{
  initializer_t()
  {
#ifdef _WIN32
    WSADATA data;
    auto ret = WSAStartup(MAKEWORD(2, 2), &data);
    if(ret != 0)
    {
      system_exception_builder_t builder;
      builder << "WSAStartup() failure: " << error_status_t(ret);
      builder.explode();
    }
    assert(LOBYTE(data.wVersion) == 2);
    assert(HIBYTE(data.wVersion) == 2);
#endif    
  }

  initializer_t(initializer_t const&) = delete;
  initializer_t& operator=(initializer_t const&) = delete;
  
  ~initializer_t()
  {
#ifdef _WIN32
    auto ret = WSACleanup();
    static_cast<void>(ret);
    assert(ret == 0);
#endif    
  }
};

socket_layer_t::socket_layer_t()
: initializer_(std::make_unique<initializer_t>())
{ }

socket_layer_t::~socket_layer_t()
{ }

} // namespace cuti
