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

#include "socket_nifty.hpp"

// Unconditionally enable asserts here
#undef NDEBUG
#include <cassert>

#ifdef _WIN32

#include <winsock2.h>

namespace xes
{

namespace // anonymous
{

struct socket_initializer_t
{
  socket_initializer_t()
  {
    WSADATA data;
    auto ret = WSAStartup(MAKEWORD(2, 2), &data);
    assert(ret == 0);
    assert(LOBYTE(data.wVersion) == 2);
    assert(HIBYTE(data.wVersion) == 2);
  }

  socket_initializer_t(socket_initializer_t const&) = delete;
  socket_initializer_t& operator=(socket_initializer_t const&) = delete;
  
  ~socket_initializer_t()
  {
     auto ret = WSACleanup();
     assert(ret == 0);
  }
};

} // anonymous

} // xes

#else // POSIX

namespace xes
{

namespace // anonymous
{

struct socket_initializer_t
{
  socket_initializer_t()
  { }

  socket_initializer_t(socket_initializer_t const&) = delete;
  socket_initializer_t& operator=(socket_initializer_t const&) = delete;
  
  ~socket_initializer_t()
  { }
};

} // anonymous

} // xes

#endif // POSIX

namespace xes
{

namespace // anonymous
{

unsigned int count = 0;
socket_initializer_t* initializer = nullptr;

} // anonymous

socket_nifty_t::socket_nifty_t()
{
  if(count++ == 0)
  {
    assert(initializer == nullptr);
    initializer = new socket_initializer_t;
  }
  else
  {
    assert(initializer != nullptr);
  }
}

socket_nifty_t::~socket_nifty_t()
{
  assert(count != 0);
  assert(initializer != nullptr);

  if(--count == 0)
  {
    delete initializer;
    initializer = nullptr;
  }
}

} // xes
