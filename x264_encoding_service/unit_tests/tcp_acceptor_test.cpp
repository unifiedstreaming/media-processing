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

#include "tcp_acceptor.hpp"
#include "tcp_connection.hpp"
#include "endpoint.hpp"
#include "logging_context.hpp"
#include "logger.hpp"
#include "resolver.hpp"
#include "streambuf_backend.hpp"
#include "system_error.hpp"

#include <chrono>
#include <iostream>
#include <thread>

// Enable assert()
#undef NDEBUG
#include <cassert>

namespace // anonymous
{

using namespace xes;

void blocking_accept(logging_context_t const& context,
                     endpoint_t const& interface)
{
  tcp_acceptor_t acceptor(interface);
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << "blocking_accept: acceptor " << acceptor <<
      " at interface " << interface;
  }

  tcp_connection_t client(acceptor.local_endpoint());
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << "client side: " << client;
  }

  auto server = acceptor.accept();
  assert(server != nullptr);
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << "server side: " << *server;
  }
}

void blocking_accept(logging_context_t const& context)
{
  auto interfaces = local_interfaces(any_port);
  assert(!interfaces.empty());
  
  for(auto const& interface : interfaces)
  {
    blocking_accept(context, interface);
  }
}

void nonblocking_accept(logging_context_t const& context,
                        endpoint_t const& interface)
{
  tcp_acceptor_t acceptor(interface);
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << "nonblocking_accept: acceptor " << acceptor <<
      " at interface " << interface;
  }
  acceptor.set_nonblocking();

  auto server = acceptor.accept();
  assert(server == nullptr);
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << acceptor << " returned expected nullptr";
  }

  tcp_connection_t client(acceptor.local_endpoint());
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << "client side: " << client;
  }

  unsigned int pause = 0;
  unsigned int attempt;
  for(attempt = 0; server == nullptr && attempt != 10; ++attempt)
  {
    if(pause != 0)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(pause));
    }
    pause = pause * 2 + 1;

    if(auto msg = context.message_at(loglevel_t::info))
    {
      *msg << acceptor << ": accept(): attempt# " << attempt + 1;
    }

    server = acceptor.accept();
  }

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << acceptor << ": " << attempt << " attempts(s)";
  }

  assert(server != nullptr);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << "server side: " << *server;
  }
}

void nonblocking_accept(logging_context_t const& context)
{
  auto interfaces = local_interfaces(any_port);
  assert(!interfaces.empty());
  
  for(auto const& interface : interfaces)
  {
    nonblocking_accept(context, interface);
  }
}

void duplicate_bind(logging_context_t const& context,
                    endpoint_t const& interface)
{
  tcp_acceptor_t acceptor1(interface);
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << "duplicate_bind: acceptor " << acceptor1 <<
      " at interface " << interface;
  }

  bool caught = false;
  try
  {
    if(auto msg = context.message_at(loglevel_t::info))
    {
      *msg << "binding to " << acceptor1;
    }
    tcp_acceptor_t acceptor2(acceptor1.local_endpoint());
  }
  catch(system_exception_t const& ex)
  {
    if(auto msg = context.message_at(loglevel_t::info))
    {
      *msg << "caught expected exception: " << ex.what();
    }
    caught = true;
  }
  assert(caught);
}

void duplicate_bind(logging_context_t const& context)
{
  auto interfaces = local_interfaces(any_port);
  assert(!interfaces.empty());
  
  for(auto const& interface : interfaces)
  {
    duplicate_bind(context, interface);
  }
}

bool prove_dual_stack(logging_context_t const& context,
                      endpoints_t const& interfaces)
{
  assert(interfaces.size() >= 2);

  /*
   * Bind to the first interface in the list
   */
  endpoint_t interface1 = interfaces.front();
  tcp_acceptor_t acceptor1(interface1);
  if(auto msg = context.message_at(loglevel_t::info))
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
  endpoint_t target = resolve_ip(interface2.ip_address(),
                                 acceptor1.local_endpoint().port());

  bool result = false;

  try
  {
    tcp_acceptor_t acceptor2(target);
    if(auto msg = context.message_at(loglevel_t::info))
    {
      *msg << "dual_stack: acceptor2 " << acceptor2 <<
        " at interface " << interface2;
    }
    result = true;
  }
  catch(system_exception_t const& ex)
  {
    if(auto msg = context.message_at(loglevel_t::info))
    {
      *msg << "failed to bind to " << target << ": " << ex.what();
    }
  }

  return result;
}
    
void dual_stack(logging_context_t const& context)
{
  // Check that we have multiple local interfaces (one for each family)
  auto interfaces = local_interfaces(any_port);
  assert(!interfaces.empty());
  
  if(interfaces.size() == 1)
  {
    if(auto msg = context.message_at(loglevel_t::info))
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
    proven = prove_dual_stack(context, interfaces);
  }

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << "dual_stack: " << attempt << " attempt(s)";
  }

  assert(proven);
}

int throwing_main(int argc, char const* const argv[])
{
  logger_t logger(argv[0]);
  logger.set_backend(std::make_unique<xes::streambuf_backend_t>(std::cerr));
  logging_context_t context(logger,
                            argc == 1 ? loglevel_t::error : loglevel_t::info);

  blocking_accept(context);
  nonblocking_accept(context);
  duplicate_bind(context);
  dual_stack(context);

  return 0;
}

} // anonymous

int main(int argc, char* argv[])
{
  int r = 1;

  try
  {
    r = throwing_main(argc, argv);
  }
  catch(std::exception const& ex)
  {
    std::cerr << argv[0] << ": exception: " << ex.what() << std::endl;
  }

  return r;
}
