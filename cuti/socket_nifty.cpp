/*
 * Copyright (C) 2021 CodeShop B.V.
 *
 * This file is part of the cuti library.
 *
 * The cuti library is free software: you can redistribute it
 * and/or modify it under the terms of version 2 of the GNU General
 * Public License as published by the Free Software Foundation.
 *
 * The cuti library is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See version 2 of the GNU General Public License for more details.
 *
 * You should have received a copy of version 2 of the GNU General
 * Public License along with the cuti library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "socket_nifty.hpp"

#include "tcp_socket.hpp"

#include <cstring>

// Unconditionally enable asserts here
#undef NDEBUG
#include <cassert>

#ifdef _WIN32

#include <winsock2.h>

namespace cuti
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

} // cuti

#else // POSIX

#include <signal.h>

namespace cuti
{

struct socket_initializer_t
{
  socket_initializer_t()
  : ignore_sigpipe_(!tcp_socket_t::stops_sigpipe())
  {
    if(ignore_sigpipe_)
    {
      // tcp_socket_t can't stop SIGPIPE; ignore it process-wide
      struct sigaction new_action;
      std::memset(&new_action, '\0', sizeof new_action);
      new_action.sa_handler = SIG_IGN;

      int r = ::sigaction(SIGPIPE, &new_action, &saved_action_);
      assert(r != -1);
    }
  }
      
  socket_initializer_t(socket_initializer_t const&) = delete;
  socket_initializer_t& operator=(socket_initializer_t const&) = delete;
  
  ~socket_initializer_t()
  {
    if(ignore_sigpipe_)
    {
      ::sigaction(SIGPIPE, &saved_action_, nullptr);
    }
  }

private :
  bool const ignore_sigpipe_;
  struct sigaction saved_action_;
};

} // cuti

#endif // POSIX

namespace cuti
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

} // cuti
