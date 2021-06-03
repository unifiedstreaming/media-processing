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

#include "tcp_socket.hpp"
#include "resolver.hpp"

#include <exception>
#include <iostream>
#include <utility>

// Enable assert()
#undef NDEBUG
#include <cassert>

#ifdef __clang__
// Clang needlessly complains about the two self-moves below, but these are
// intended, so silence the warning.
#pragma clang diagnostic ignored "-Wself-move"
#endif

namespace // anonymous
{

void socket_state_for_family(int family)
{
  cuti::tcp_socket_t sock0;
  assert(sock0.empty());

  cuti::tcp_socket_t sock1(family);
  assert(!sock1.empty());

  cuti::tcp_socket_t sock2 = std::move(sock1);
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
  auto interfaces = cuti::local_interfaces(cuti::any_port);
  for(auto const& interface : interfaces)
  {
    socket_state_for_family(interface.address_family());
  }
}

void run_tests(int, char const* const*)
{
  socket_state();
}

} // anonymous

int main(int argc, char* argv[])
{
  try
  {
    run_tests(argc, argv);
  }
  catch(std::exception const& ex)
  {
    std::cerr << argv[0] << ": exception: " << ex.what() << std::endl;
    throw;
  }

  return 0;
}

