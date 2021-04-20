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

#include "tcp_acceptor.hpp"

#include "endpoint.hpp"
#include "logging_context.hpp"
#include "logger.hpp"
#include "poll_selector.hpp"
#include "resolver.hpp"
#include "selector.hpp"
#include "streambuf_backend.hpp"
#include "system_error.hpp"
#include "tcp_connection.hpp"

#include <chrono>
#include <iostream>
#include <thread>

// Enable assert()
#undef NDEBUG
#include <cassert>

namespace // anonymous
{

using namespace cuti;

struct selector_factory_t
{
  selector_factory_t(std::string name,
                     std::unique_ptr<selector_t>(*creator)())
  : name_(std::move(name))
  , creator_(creator)
  { }

  std::string const& name() const
  {
    return name_;
  }

  std::unique_ptr<selector_t> operator()() const
  {
    return (*creator_)();
  }

private :
  std::string name_;
  std::unique_ptr<selector_t>(*creator_)();
};

std::ostream& operator<<(std::ostream& os, selector_factory_t const& factory)
{
  os << factory.name();
  return os;
}

std::vector<selector_factory_t> available_selector_factories()
{
  std::vector<selector_factory_t> result;

#ifndef _WIN32
  result.emplace_back("poll_selector", create_poll_selector);
#endif

  return result;
}
  
int selector_timeout(std::chrono::system_clock::duration timeout)
{
  static auto zero = std::chrono::system_clock::duration::zero();
  assert(timeout >= zero);

  int result;

  if(timeout == zero)
  {
    // a true poll
    result = 0; 
  }
  else
  {
    // not a true poll
    auto count = std::chrono::duration_cast<
      std::chrono::milliseconds>(timeout).count();
    if(count < 1)
    {
      result = 1; // prevent spinloop
    }
    else if(count < 30000)
    {
      result = static_cast<int>(count);
    }
    else
    {
      result = 30000;
    }
  }

  return result;
}    
      
void run_selector(logging_context_t& context,
                  selector_t& selector,
                  int timeout_millis)
{
  assert(timeout_millis >= 0);

  auto now = std::chrono::system_clock::now();
  auto const limit = now + std::chrono::milliseconds(timeout_millis);

  do
  {
    if(!selector.has_work())
    {
      break;
    }

    timeout_millis = selector_timeout(limit - now);
    if(auto msg = context.message_at(loglevel_t::info))
    {
      *msg << "run_selector(): awaiting callback for " <<
        timeout_millis << " milliseconds...";
    }
    
    auto callback = selector.select(selector_timeout(limit - now));
    if(callback == nullptr)
    {
      if(auto msg = context.message_at(loglevel_t::info))
      {
        *msg << "run_selector(): got empty callback";
      }
    }
    else
    {
      if(auto msg = context.message_at(loglevel_t::info))
      {
        *msg << "run_selector(): invoking callback";
      }
      callback();
    }

    now = std::chrono::system_clock::now();

  } while(now < limit);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    if(selector.has_work())
    {
      *msg << "run_selector(): timeout";
    }
    else
    {
      *msg << "run_selector(): out of work";
    }
  }
}

void start_accept(logging_context_t& context,
                  io_scheduler_t& scheduler,
                  tcp_acceptor_t& acceptor)
{
  auto connection = acceptor.accept();
  if(connection == nullptr)
  {
    if(auto msg = context.message_at(loglevel_t::info))
    {
      *msg << "start_accept() " << acceptor <<
        ": no connection yet; rescheduling...";
    }
    acceptor.call_when_ready(
      [&] { start_accept(context, scheduler, acceptor); },
      scheduler);
  }
  else
  {
    if(auto msg = context.message_at(loglevel_t::info))
    {
      *msg << "start_accept(): accepted connection " << *connection;
    }
  }
}    

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

void empty_selector(logging_context_t& context,
                    selector_factory_t const& factory)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << "empty_selector(): factory: " << factory;
  }

  auto selector = factory();
  run_selector(context, *selector, 60000);
  assert(!selector->has_work());
}

void empty_selector(logging_context_t& context)
{
  auto factories = available_selector_factories();
  for(auto const& factory : factories)
  {
    empty_selector(context, factory);
  }
}

void no_client(logging_context_t& context,
               selector_factory_t const& factory,
               endpoint_t const& interface)
{
  auto selector = factory();
  tcp_acceptor_t acceptor(interface);
  acceptor.set_nonblocking();

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << "no_client(): factory: " << factory << " acceptor: " << acceptor;
  }
  start_accept(context, *selector, acceptor);

  run_selector(context, *selector, 10);
  assert(selector->has_work());
}

void no_client(logging_context_t& context)
{
  auto factories = available_selector_factories();
  auto interfaces = local_interfaces(any_port);

  for(auto const& factory : factories)
  {
    for(auto const& interface : interfaces)
    {
      no_client(context, factory, interface);
    }
  }
}

void single_client(logging_context_t& context,
                   selector_factory_t const& factory,
                   endpoint_t const& interface)
{
  auto selector = factory();
  tcp_acceptor_t acceptor(interface);
  acceptor.set_nonblocking();

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << "single_client(): factory: " << factory <<
      " acceptor: " << acceptor;
  }
  start_accept(context, *selector, acceptor);

  run_selector(context, *selector, 10);
  assert(selector->has_work());

  tcp_connection_t client(acceptor.local_endpoint());
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << "single_client(): client " << client;
  }

  run_selector(context, *selector, 60000);
  assert(!selector->has_work());
}

void single_client(logging_context_t& context)
{
  auto factories = available_selector_factories();
  auto interfaces = local_interfaces(any_port);

  for(auto const& factory : factories)
  {
    for(auto const& interface : interfaces)
    {
      single_client(context, factory, interface);
    }
  }
}

void multiple_clients(logging_context_t& context,
                      selector_factory_t const& factory,
                      endpoint_t const& interface)
{
  auto selector = factory();
  tcp_acceptor_t acceptor(interface);
  acceptor.set_nonblocking();

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << "multiple_clients(): factory: " << factory <<
      " acceptor: " << acceptor;
  }

  start_accept(context, *selector, acceptor);
  run_selector(context, *selector, 10);
  assert(selector->has_work());

  tcp_connection_t client1(acceptor.local_endpoint());
  tcp_connection_t client2(acceptor.local_endpoint());
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << "multiple_clients(): client1: " << client1 <<
      " client2: " << client2;
  }

  run_selector(context, *selector, 60000);
  assert(!selector->has_work());

  start_accept(context, *selector, acceptor);
  run_selector(context, *selector, 60000);
  assert(!selector->has_work());
}

void multiple_clients(logging_context_t& context)
{
  auto factories = available_selector_factories();
  auto interfaces = local_interfaces(any_port);

  for(auto const& factory : factories)
  {
    for(auto const& interface : interfaces)
    {
      multiple_clients(context, factory, interface);
    }
  }
}

void multiple_acceptors(logging_context_t& context,
                        selector_factory_t const& factory,
                        endpoint_t const& interface)
{
  auto selector = factory();

  tcp_acceptor_t acceptor1(interface);
  acceptor1.set_nonblocking();

  tcp_acceptor_t acceptor2(interface);
  acceptor2.set_nonblocking();

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << "multiple_accceptors(): factory: " << factory <<
      " acceptor1: " << acceptor1 << " acceptor2: " << acceptor2;
  }
  start_accept(context, *selector, acceptor1);
  start_accept(context, *selector, acceptor2);

  run_selector(context, *selector, 10);
  assert(selector->has_work());

  tcp_connection_t client1(acceptor1.local_endpoint());
  tcp_connection_t client2(acceptor2.local_endpoint());
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << "multiple_acceptors(): client1: " << client1 <<
      " client2: " << client2;
  }

  run_selector(context, *selector, 60000);
  assert(!selector->has_work());
}

void multiple_acceptors(logging_context_t& context)
{
  auto factories = available_selector_factories();
  auto interfaces = local_interfaces(any_port);

  for(auto const& factory : factories)
  {
    for(auto const& interface : interfaces)
    {
      multiple_acceptors(context, factory, interface);
    }
  }
}

void selector_switch(logging_context_t& context,
                     selector_factory_t const& factory,
                     endpoint_t const& interface)
{
  auto selector1 = factory();
  auto selector2 = factory();
  tcp_acceptor_t acceptor(interface);
  acceptor.set_nonblocking();

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << "selector_switch(): factory: " << factory <<
      " acceptor: " << acceptor;
  }

  assert(!selector1->has_work());
  assert(!selector2->has_work());

  acceptor.call_when_ready([] { }, *selector1);
  assert(selector1->has_work());
  assert(!selector2->has_work());

  acceptor.call_when_ready([] { }, *selector2);
  assert(!selector1->has_work());
  assert(selector2->has_work());

  acceptor.cancel_when_ready();
  assert(!selector1->has_work());
  assert(!selector2->has_work());
}

void selector_switch(logging_context_t& context)
{
  auto factories = available_selector_factories();
  auto interfaces = local_interfaces(any_port);

  for(auto const& factory : factories)
  {
    for(auto const& interface : interfaces)
    {
      selector_switch(context, factory, interface);
    }
  }
}

int throwing_main(int argc, char const* const argv[])
{
  logger_t logger(argv[0]);
  logger.set_backend(std::make_unique<cuti::streambuf_backend_t>(std::cerr));
  logging_context_t context(logger,
                            argc == 1 ? loglevel_t::error : loglevel_t::info);

  blocking_accept(context);
  nonblocking_accept(context);
  duplicate_bind(context);
  dual_stack(context);

  empty_selector(context);
  no_client(context);
  single_client(context);
  multiple_clients(context);
  multiple_acceptors(context);
  selector_switch(context);

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
