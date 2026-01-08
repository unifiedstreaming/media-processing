/*
 * Copyright (C) 2021-2026 CodeShop B.V.
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

#include <cuti/tcp_acceptor.hpp>

#include <cuti/chrono_types.hpp>
#include <cuti/endpoint.hpp>
#include <cuti/logging_context.hpp>
#include <cuti/logger.hpp>
#include <cuti/resolver.hpp>
#include <cuti/socket_layer.hpp>
#include <cuti/streambuf_backend.hpp>
#include <cuti/system_error.hpp>
#include <cuti/tcp_connection.hpp>

#include <iostream>
#include <memory>
#include <thread>

// Enable assert()
#undef NDEBUG
#include <cassert>

namespace // anonymous
{

using namespace cuti;

loglevel_t const loglevel = loglevel_t::info;

void blocking_accept(logging_context_t const& context,
                     socket_layer_t& sockets,
                     endpoint_t const& interface)
{
  tcp_acceptor_t acceptor(sockets, interface);
  if(auto msg = context.message_at(loglevel))
  {
    *msg << "blocking_accept: acceptor " << acceptor <<
      " at interface " << interface;
  }

  tcp_connection_t client(sockets, acceptor.local_endpoint());
  if(auto msg = context.message_at(loglevel))
  {
    *msg << "client side: " << client;
  }

  std::unique_ptr<tcp_connection_t> server;
  int r = acceptor.accept(server);
  assert(r == 0);
  assert(server != nullptr);
  if(auto msg = context.message_at(loglevel))
  {
    *msg << "server side: " << *server;
  }
}

void blocking_accept(logging_context_t const& context)
{
  socket_layer_t sockets;

  auto interfaces = local_interfaces(sockets, any_port);
  assert(!interfaces.empty());

  for(auto const& interface : interfaces)
  {
    blocking_accept(context, sockets, interface);
  }
}

void nonblocking_accept(logging_context_t const& context,
                        socket_layer_t& sockets,
                        endpoint_t const& interface)
{
  tcp_acceptor_t acceptor(sockets, interface);
  if(auto msg = context.message_at(loglevel))
  {
    *msg << "nonblocking_accept: acceptor " << acceptor <<
      " at interface " << interface;
  }
  acceptor.set_nonblocking();

  std::unique_ptr<tcp_connection_t> server;
  int r = acceptor.accept(server);
  assert(r == 0);
  assert(server == nullptr);
  if(auto msg = context.message_at(loglevel))
  {
    *msg << acceptor << " returned expected nullptr";
  }

  tcp_connection_t client(sockets, acceptor.local_endpoint());
  if(auto msg = context.message_at(loglevel))
  {
    *msg << "client side: " << client;
  }

  unsigned int pause = 0;
  unsigned int attempt;
  for(attempt = 0; server == nullptr && attempt != 10; ++attempt)
  {
    if(pause != 0)
    {
      std::this_thread::sleep_for(milliseconds_t(pause));
    }
    pause = pause * 2 + 1;

    if(auto msg = context.message_at(loglevel))
    {
      *msg << acceptor << ": accept(): attempt# " << attempt + 1;
    }

    r = acceptor.accept(server);
    assert(r == 0);
  }

  if(auto msg = context.message_at(loglevel))
  {
    *msg << acceptor << ": " << attempt << " attempts(s)";
  }

  assert(server != nullptr);

  if(auto msg = context.message_at(loglevel))
  {
    *msg << "server side: " << *server;
  }
}

void nonblocking_accept(logging_context_t const& context)
{
  socket_layer_t sockets;
  auto interfaces = local_interfaces(sockets, any_port);
  assert(!interfaces.empty());

  for(auto const& interface : interfaces)
  {
    nonblocking_accept(context, sockets, interface);
  }
}

void duplicate_bind(logging_context_t const& context,
                    socket_layer_t& sockets,
                    endpoint_t const& interface)
{
  tcp_acceptor_t acceptor1(sockets, interface);
  if(auto msg = context.message_at(loglevel))
  {
    *msg << "duplicate_bind: acceptor " << acceptor1 <<
      " at interface " << interface;
  }

  bool caught = false;
  try
  {
    if(auto msg = context.message_at(loglevel))
    {
      *msg << "binding to " << acceptor1;
    }
    tcp_acceptor_t acceptor2(sockets, acceptor1.local_endpoint());
  }
  catch(system_exception_t const& ex)
  {
    if(auto msg = context.message_at(loglevel))
    {
      *msg << "caught expected exception: " << ex.what();
    }
    caught = true;
  }
  assert(caught);
}

void duplicate_bind(logging_context_t const& context)
{
  socket_layer_t sockets;
  auto interfaces = local_interfaces(sockets, any_port);
  assert(!interfaces.empty());

  for(auto const& interface : interfaces)
  {
    duplicate_bind(context, sockets, interface);
  }
}

bool prove_dual_stack(logging_context_t const& context,
                      socket_layer_t& sockets,
                      endpoints_t const& interfaces)
{
  assert(interfaces.size() >= 2);

  /*
   * Bind to the first interface in the list
   */
  endpoint_t interface1 = interfaces.front();
  tcp_acceptor_t acceptor1(sockets, interface1);
  if(auto msg = context.message_at(loglevel))
  {
    *msg << "dual_stack: acceptor1 " << acceptor1 <<
      " at interface " << interface1;
  }

  /*
   * Try to bind to the IP address for the second interface, using the
   * port number of the acceptor bound to the first interface. There
   * is a small chance that this address is in use, so failing to bind
   * is not necessarily an error. However, if we succeed, we have
   * proven that our dual stack works.
   */
  endpoint_t interface2 = interfaces.back();
  assert(interface1.address_family() != interface2.address_family());
  endpoint_t target = resolve_ip(sockets,
                                 interface2.ip_address(),
                                 acceptor1.local_endpoint().port());

  bool result = false;

  try
  {
    tcp_acceptor_t acceptor2(sockets, target);
    if(auto msg = context.message_at(loglevel))
    {
      *msg << "dual_stack: acceptor2 " << acceptor2 <<
        " at interface " << interface2;
    }
    result = true;
  }
  catch(system_exception_t const& ex)
  {
    if(auto msg = context.message_at(loglevel))
    {
      *msg << "failed to bind to " << target << ": " << ex.what();
    }
  }

  return result;
}

void dual_stack(logging_context_t const& context)
{
  socket_layer_t sockets;

  // Check that we have multiple local interfaces (one for each family)
  auto interfaces = local_interfaces(sockets, any_port);
  assert(!interfaces.empty());

  if(interfaces.size() == 1)
  {
    if(auto msg = context.message_at(loglevel))
    {
      *msg << "dual_stack: single local interface - can't test";
    }
    return;
  }

  // Because of the (small) chance of a false negative, we try multiple times
  bool proven = false;
  unsigned int attempt;
  for(attempt = 0; !proven && attempt != 10; ++attempt)
  {
    proven = prove_dual_stack(context, sockets, interfaces);
  }

  if(auto msg = context.message_at(loglevel))
  {
    *msg << "dual_stack: " << attempt << " attempt(s)";
  }

  assert(proven);
}

void run_tests(int argc, char const* const* /* argv */)
{
  logger_t logger(std::make_unique<cuti::streambuf_backend_t>(std::cerr));
  logging_context_t context(
    logger, argc == 1 ? loglevel_t::error : loglevel_t::info);

  blocking_accept(context);
  nonblocking_accept(context);
  duplicate_bind(context);
  dual_stack(context);
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
