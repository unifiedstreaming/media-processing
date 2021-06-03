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

#include "resolver.hpp"
#include "system_error.hpp"

#include <exception>
#include <iostream>

// enable assert()
#undef NDEBUG
#include <cassert>

#undef PRINT
// #define PRINT 1

namespace // anonymous
{

using namespace cuti;

void check_endpoint(endpoint_t const& ep)
{
  endpoint_t refetched = resolve_ip(ep.ip_address(), ep.port());

  assert(refetched.address_family() == ep.address_family());
  assert(refetched.socket_address_size() == ep.socket_address_size());
  assert(refetched.ip_address() == ep.ip_address());
  assert(refetched.port() == ep.port());
}

void ip_address()
{
  auto interfaces = local_interfaces(any_port);
  for(auto const& interface: interfaces)
  {
    auto ip_address = interface.ip_address();
    endpoint_t ep = resolve_ip(ip_address, any_port);
#if PRINT
      std::cout << "ip_addresses(): " <<
        ip_address << " -> " << ep << std::endl;
#endif
    assert(ep.port() == any_port);
  }
}

void not_an_ip_address()
{
  bool caught = false;

  try
  {
    endpoint_t ep = resolve_ip("localhost", any_port);
  }
  catch(system_exception_t const& ex)
  {
#if PRINT
    std::cout << "not_an_ip_address(): caught expected exception: " <<
      ex.what() << std::endl;
#else
    static_cast<void>(ex);
#endif
    caught = true;
  }

  assert(caught);
}

void local_endpoints()
{
  auto endpoints = local_interfaces(any_port);
  assert(!endpoints.empty());

  for(auto const& ep : endpoints)
  {
#if PRINT
    std::cout << "local interfaces: " << ep << std::endl;
#endif
    assert(ep.port() == any_port);
    check_endpoint(ep);
  }
}

void local_endpoints_with_port()
{
  auto endpoints = local_interfaces(11264);
  assert(!endpoints.empty());

  for(auto const& ep : endpoints)
  {
#if PRINT
    std::cout << "local interfaces port 11264: " << ep << std::endl;
#endif
    assert(ep.port() == 11264);
    check_endpoint(ep);
  }
}

void all_endpoints()
{
  auto endpoints = all_interfaces(any_port);
  assert(!endpoints.empty());

  for(auto const& ep : endpoints)
  {
#if PRINT
    std::cout << "all interfaces: " << ep << std::endl;
#endif
    assert(ep.port() == any_port);
    check_endpoint(ep);
  }
}

void all_endpoints_with_port()
{
  auto endpoints = all_interfaces(11264);
  assert(!endpoints.empty());

  for(auto const& ep : endpoints)
  {
#if PRINT
    std::cout << "all interfaces port 11264: " << ep << std::endl;
#endif
    assert(ep.port() == 11264);
    check_endpoint(ep);
  }
}

void localhost()
{
  auto endpoints = resolve_host("localhost", any_port);
  assert(!endpoints.empty());

  for(auto const& ep : endpoints)
  {
#if PRINT
    std::cout << "localhost: " << ep << std::endl;
#endif
    assert(ep.port() == any_port);
    check_endpoint(ep);
  }
}

void localhost_with_port()
{
  auto endpoints = resolve_host("localhost", 11264);
  assert(!endpoints.empty());

  for(auto const& ep : endpoints)
  {
#if PRINT
    std::cout << "localhost port 11264: " << ep << std::endl;
#endif
    assert(ep.port() == 11264);
    check_endpoint(ep);
  }
}

void remote_host()
{
  auto endpoints = resolve_host("a.root-servers.net", any_port);
  assert(!endpoints.empty());

  for(auto const& ep : endpoints)
  {
#if PRINT
    std::cout << "a.root-servers.net: " << ep << std::endl;
#endif
    assert(ep.port() == any_port);
    check_endpoint(ep);
  }
}

void remote_host_with_port()
{
  auto endpoints = resolve_host("a.root-servers.net", 53);
  assert(!endpoints.empty());

  for(auto const& ep : endpoints)
  {
#if PRINT
    std::cout << "a.root-servers.net port 53: " << ep << std::endl;
#endif
    assert(ep.port() == 53);
    check_endpoint(ep);
  }
}

void unknown_host()
{
  bool caught = false;
  try
  {
    auto endpoints = resolve_host("mail.dev.null", any_port);
  }
  catch(std::exception const& ex)
  {
#if PRINT
    std::cout << "unknown_host(): caught expected exception: " <<
      ex.what() << std::endl;
#else
    static_cast<void>(ex); // suppress compiler warning
#endif
    caught = true;
  }
  assert(caught);
}

void unknown_host_with_port()
{
  bool caught = false;
  try
  {
    auto endpoints = resolve_host("mail.dev.null", 25);
  }
  catch(std::exception const& ex)
  {
#if PRINT
    std::cout << "unknown_host_with_port(): caught expected exception: " <<
      ex.what() << std::endl;
#else
    static_cast<void>(ex); // suppress compiler warning
#endif
    caught = true;
  }
  assert(caught);
}

void run_tests(int, char const* const*)
{
  ip_address();
  not_an_ip_address();
  local_endpoints();
  local_endpoints_with_port();
  all_endpoints();
  all_endpoints_with_port();
  localhost();
  localhost_with_port();
  remote_host();
  remote_host_with_port();
  unknown_host();
  unknown_host_with_port();

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
