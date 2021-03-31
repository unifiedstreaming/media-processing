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

#include "endpoint_list.hpp"

#include <iostream>

// enable assert()
#undef NDEBUG
#include <cassert>

#undef PRINT
// #define PRINT 1

namespace // anonymous
{

void check_endpoint(xes::endpoint_t const& ep)
{
  xes::endpoint_list_t list(xes::ip_address(ep), xes::port_number(ep));
  auto it = list.begin();

  assert(it != list.end());
  assert(xes::address_family(*it) == xes::address_family(ep));
  assert(xes::ip_address(*it) == xes::ip_address(ep));
  assert(xes::port_number(*it) == xes::port_number(ep));

  ++it;
  assert(it == list.end());
}
  
void empty_list()
{
  xes::endpoint_list_t list;
  assert(list.empty());
  assert(list.begin() == list.end());
}

void local_interfaces()
{
  xes::endpoint_list_t list(xes::local_interfaces, xes::any_port);
  assert(!list.empty());

  for(auto const& ep : list)
  {
#if PRINT
    std::cout << "local interfaces: " <<
      xes::ip_address(ep) << ' ' << xes::port_number(ep) << std::endl;
#endif
    assert(xes::port_number(ep) == xes::any_port);
    check_endpoint(ep);
  }
}

void local_interfaces_with_port()
{
  xes::endpoint_list_t list(xes::local_interfaces, 11264);
  assert(!list.empty());

  for(auto const& ep : list)
  {
#if PRINT
    std::cout << "local interfaces port 11264: " <<
      xes::ip_address(ep) << ' ' << xes::port_number(ep) << std::endl;
#endif
    assert(xes::port_number(ep) == 11264);
    check_endpoint(ep);
  }
}

void all_interfaces()
{
  xes::endpoint_list_t list(xes::all_interfaces, xes::any_port);
  assert(!list.empty());

  for(auto const& ep : list)
  {
#if PRINT
    std::cout << "all interfaces: " <<
      xes::ip_address(ep) << ' ' << xes::port_number(ep) << std::endl;
#endif
    assert(xes::port_number(ep) == xes::any_port);
    check_endpoint(ep);
  }
}

void all_interfaces_with_port()
{
  xes::endpoint_list_t list(xes::all_interfaces, 11264);
  assert(!list.empty());

  for(auto const& ep : list)
  {
#if PRINT
    std::cout << "all interfaces port 11264: " <<
      xes::ip_address(ep) << ' ' << xes::port_number(ep) << std::endl;
#endif
    assert(xes::port_number(ep) == 11264);
    check_endpoint(ep);
  }
}

void localhost()
{
  xes::endpoint_list_t list("localhost", xes::any_port);
  assert(!list.empty());

  for(auto const& ep : list)
  {
#if PRINT
    std::cout << "localhost: " <<
      xes::ip_address(ep) << ' ' << xes::port_number(ep) << std::endl;
#endif
    assert(xes::port_number(ep) == xes::any_port);
    check_endpoint(ep);
  }
}

void localhost_with_port()
{
  xes::endpoint_list_t list("localhost", 11264);
  assert(!list.empty());

  for(auto const& ep : list)
  {
#if PRINT
    std::cout << "localhost port 11264: " <<
      xes::ip_address(ep) << ' ' << xes::port_number(ep) << std::endl;
#endif
    assert(xes::port_number(ep) == 11264);
    check_endpoint(ep);
  }
}

void remote_host()
{
  xes::endpoint_list_t list("a.root-servers.net", xes::any_port);
  assert(!list.empty());

  for(auto const& ep : list)
  {
#if PRINT
    std::cout << "a.root-servers.net: " <<
      xes::ip_address(ep) << ' ' << xes::port_number(ep) << std::endl;
#endif
    assert(xes::port_number(ep) == xes::any_port);
    check_endpoint(ep);
  }
}

void remote_host_with_port()
{
  xes::endpoint_list_t list("a.root-servers.net", 53);
  assert(!list.empty());

  for(auto const& ep : list)
  {
#if PRINT
    std::cout << "a.root-servers.net port 53: " <<
      xes::ip_address(ep) << ' ' << xes::port_number(ep) << std::endl;
#endif
    assert(xes::port_number(ep) == 53);
    check_endpoint(ep);
  }
}

void unknown_host()
{
  bool caught = false;
  try
  {
    xes::endpoint_list_t list("mail.dev.null", xes::any_port);
  }
  catch(std::exception const& ex)
  {
#if PRINT
    std::cout << ex.what() << std::endl;
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
    xes::endpoint_list_t list("mail.dev.null", 25);
  }
  catch(std::exception const& ex)
  {
#if PRINT
    std::cout << ex.what() << std::endl;
#else
    static_cast<void>(ex); // suppress compiler warning
#endif
    caught = true;
  }
  assert(caught);
}
  
} // anonymous

int main()
{
  empty_list();
  local_interfaces();
  local_interfaces_with_port();
  all_interfaces();
  all_interfaces_with_port();
  localhost();
  localhost_with_port();
  remote_host();
  remote_host_with_port();
  unknown_host();
  unknown_host_with_port();

  return 0;
}
