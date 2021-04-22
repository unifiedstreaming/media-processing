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
#include "resolver.hpp"
#include "selector.hpp"
#include "selector_factory.hpp"
#include "streambuf_backend.hpp"
#include "system_error.hpp"
#include "tcp_connection.hpp"

#include <chrono>
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

/*
 * The ultimate denial-of-service protector accepts and kills up to
 * <count> incoming connections on a randomly chosen local endpoint.
 */
struct dos_protector_t
{
  dos_protector_t(logging_context_t& context,
                  endpoint_t const& interface,
                  int count)
  : context_(context)
  , acceptor_(interface)
  , count_(count)
  {
    acceptor_.set_nonblocking();
  }

  endpoint_t const& local_endpoint() const
  {
    return acceptor_.local_endpoint();
  }

  // SSTS: static start takes shared
  static endpoint_t start(std::shared_ptr<dos_protector_t> const& self,
                          io_scheduler_t& scheduler)
  {
    if(self->count_ > 0)
    {
      if(auto msg = self->context_.message_at(loglevel))
      {
        *msg << "dos_protector: " << self->acceptor_ <<
	   ": requesting on_ready() callback";
      }
      self->acceptor_.call_when_ready(scheduler,
        [self, &scheduler] { on_ready(self, scheduler); });
    }
    return self->local_endpoint();
  }
      
  ~dos_protector_t()
  {
    if(auto msg = context_.message_at(loglevel))
    {
      *msg << "dos_protector: " << acceptor_ << ": destructor";
    }
  }
  
private :
  static void on_ready(std::shared_ptr<dos_protector_t> const& self,
                       io_scheduler_t& scheduler)
  {
    assert(self->count_ > 0);

    auto incoming = self->acceptor_.accept();
    if(incoming == nullptr)
    {
      if(auto msg = self->context_.message_at(loglevel))
      {
        *msg << "dos_protector: " << self->acceptor_ <<
          ": no incoming connection yet";
      }
    }
    else
    {
      if(auto msg = self->context_.message_at(loglevel))
      {
        *msg << "dos_protector: " << self->acceptor_ <<
          ": killing incoming connection " << *incoming;
      }
      --self->count_;
    }

    start(self, scheduler);
  }

private :
  logging_context_t& context_;
  tcp_acceptor_t acceptor_;
  int count_;
};

// SSTS: static start takes shared
template<typename... Args>
endpoint_t start_dos_protector(io_scheduler_t& scheduler, Args&&... args)
{
  return start_io_handler<dos_protector_t>(
    scheduler, std::forward<Args>(args)...);
}
  
void blocking_accept(logging_context_t const& context,
                     endpoint_t const& interface)
{
  tcp_acceptor_t acceptor(interface);
  if(auto msg = context.message_at(loglevel))
  {
    *msg << "blocking_accept: acceptor " << acceptor <<
      " at interface " << interface;
  }

  tcp_connection_t client(acceptor.local_endpoint());
  if(auto msg = context.message_at(loglevel))
  {
    *msg << "client side: " << client;
  }

  auto server = acceptor.accept();
  assert(server != nullptr);
  if(auto msg = context.message_at(loglevel))
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
  if(auto msg = context.message_at(loglevel))
  {
    *msg << "nonblocking_accept: acceptor " << acceptor <<
      " at interface " << interface;
  }
  acceptor.set_nonblocking();

  auto server = acceptor.accept();
  assert(server == nullptr);
  if(auto msg = context.message_at(loglevel))
  {
    *msg << acceptor << " returned expected nullptr";
  }

  tcp_connection_t client(acceptor.local_endpoint());
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
      std::this_thread::sleep_for(std::chrono::milliseconds(pause));
    }
    pause = pause * 2 + 1;

    if(auto msg = context.message_at(loglevel))
    {
      *msg << acceptor << ": accept(): attempt# " << attempt + 1;
    }

    server = acceptor.accept();
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
    tcp_acceptor_t acceptor2(acceptor1.local_endpoint());
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
  endpoint_t target = resolve_ip(interface2.ip_address(),
                                 acceptor1.local_endpoint().port());

  bool result = false;

  try
  {
    tcp_acceptor_t acceptor2(target);
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
  // Check that we have multiple local interfaces (one for each family)
  auto interfaces = local_interfaces(any_port);
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
    proven = prove_dual_stack(context, interfaces);
  }

  if(auto msg = context.message_at(loglevel))
  {
    *msg << "dual_stack: " << attempt << " attempt(s)";
  }

  assert(proven);
}

void empty_selector(logging_context_t& context,
                    selector_factory_t const& factory)
{
  if(auto msg = context.message_at(loglevel))
  {
    *msg << "empty_selector(): using " << factory << " selector";
  }

  auto selector = factory();
  run_selector(context, loglevel, *selector, std::chrono::seconds(60));
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
  endpoint_t protector = start_dos_protector(*selector, context, interface, 1);

  if(auto msg = context.message_at(loglevel))
  {
    *msg << "no_client(): using " << factory <<
      " selector; protector: " << protector;
  }

  run_selector(context, loglevel, *selector, std::chrono::seconds(0));
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
  endpoint_t protector = start_dos_protector(*selector, context, interface, 1);

  if(auto msg = context.message_at(loglevel))
  {
    *msg << "single_client(): using " << factory <<
      " selector; protector: " << protector;
  }

  run_selector(context, loglevel, *selector, std::chrono::seconds(0));
  assert(selector->has_work());

  tcp_connection_t client(protector);
  if(auto msg = context.message_at(loglevel))
  {
    *msg << "single_client(): client " << client;
  }

  run_selector(context, loglevel, *selector, std::chrono::seconds(60));
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
  endpoint_t protector = start_dos_protector(*selector, context, interface, 2);

  if(auto msg = context.message_at(loglevel))
  {
    *msg << "multiple_clients(): using " << factory <<
      " selector; protector: " << protector;
  }

  run_selector(context, loglevel, *selector, std::chrono::seconds(0));
  assert(selector->has_work());

  tcp_connection_t client1(protector);
  tcp_connection_t client2(protector);
  if(auto msg = context.message_at(loglevel))
  {
    *msg << "multiple_clients(): client1: " << client1 <<
      " client2: " << client2;
  }

  run_selector(context, loglevel, *selector, std::chrono::seconds(60));
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
  endpoint_t protector1 = start_dos_protector(
    *selector, context, interface, 1);
  endpoint_t protector2 = start_dos_protector(
    *selector, context, interface, 1);

  if(auto msg = context.message_at(loglevel))
  {
    *msg << "multiple_acceptors(): using " << factory <<
      " selector; protector1: " << protector1 <<
      " protector2: " << protector2;
  }

  run_selector(context, loglevel, *selector, std::chrono::seconds(0));
  assert(selector->has_work());

  tcp_connection_t client1(protector1);
  tcp_connection_t client2(protector2);
  if(auto msg = context.message_at(loglevel))
  {
    *msg << "multiple_acceptors(): client1: " << client1 <<
      " client2: " << client2;
  }

  run_selector(context, loglevel, *selector, std::chrono::seconds(60));
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

  if(auto msg = context.message_at(loglevel))
  {
    *msg << "selector_switch(): using " << factory <<
      " selector; acceptor: " << acceptor;
  }

  assert(!selector1->has_work());
  assert(!selector2->has_work());

  int ticket = acceptor.call_when_ready(*selector1, [] { });
  assert(selector1->has_work());
  assert(!selector2->has_work());

  acceptor.cancel_when_ready(*selector1, ticket);
  ticket = acceptor.call_when_ready(*selector2, [] { });
  assert(!selector1->has_work());
  assert(selector2->has_work());

  acceptor.cancel_when_ready(*selector2, ticket);
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
  logging_context_t context(
    logger, argc == 1 ? loglevel_t::error : loglevel_t::info);

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
