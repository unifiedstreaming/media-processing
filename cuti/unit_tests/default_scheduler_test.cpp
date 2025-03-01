/*
 * Copyright (C) 2021-2025 CodeShop B.V.
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

#include <cuti/default_scheduler.hpp>

#include <cuti/chrono_types.hpp>
#include <cuti/endpoint.hpp>
#include <cuti/logging_context.hpp>
#include <cuti/logger.hpp>
#include <cuti/resolver.hpp>
#include <cuti/selector.hpp>
#include <cuti/selector_factory.hpp>
#include <cuti/socket_layer.hpp>
#include <cuti/stack_marker.hpp>
#include <cuti/streambuf_backend.hpp>
#include <cuti/tcp_acceptor.hpp>
#include <cuti/tcp_connection.hpp>

#include <exception>
#include <iostream>
#include <memory>

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
  dos_protector_t(logging_context_t const& context,
                  socket_layer_t& sockets,
                  endpoint_t const& interface,
                  int count,
                  duration_t timeout = minutes_t(1))
  : context_(context)
  , acceptor_(sockets, interface)
  , count_(count)
  , timed_out_(false)
  , timeout_(timeout)
  , ready_ticket_()
  , timeout_ticket_()
  {
    acceptor_.set_nonblocking();
  }

  endpoint_t const& local_endpoint() const
  {
    return acceptor_.local_endpoint();
  }

  bool done() const
  {
    return count_ <= 0;
  }

  bool timed_out() const
  {
    return timed_out_;
  }

  // SSTS: static start takes shared
  static void start(std::shared_ptr<dos_protector_t> const& self,
                    scheduler_t& scheduler)
  {
    self->do_start(self, scheduler);
  }

  ~dos_protector_t()
  {
    if(auto msg = context_.message_at(loglevel))
    {
      *msg << "dos_protector: " << acceptor_ << ": destructor";
    }
  }

private :
  void do_start(std::shared_ptr<dos_protector_t> const& self,
                scheduler_t& scheduler)
  {
    assert(self.get() == this);
    assert(ready_ticket_.empty());
    assert(timeout_ticket_.empty());

    if(!timed_out_ && count_ > 0)
    {
      if(auto msg = context_.message_at(loglevel))
      {
        *msg << "dos_protector: " << acceptor_ <<
           ": requesting ready callback";
      }
      ready_ticket_ = acceptor_.call_when_ready(
        scheduler,
        [self, &scheduler](stack_marker_t&)
        { self->on_ready(self, scheduler); }
      );

      if(auto msg = context_.message_at(loglevel))
      {
        *msg << "dos_protector: " << acceptor_ <<
           ": requesting timeout callback";
      }
      timeout_ticket_ = scheduler.call_alarm(
        timeout_, 
        [self, &scheduler](stack_marker_t&)
        { self->on_timeout(self, scheduler); }
      );
    }
  }

  void on_ready(std::shared_ptr<dos_protector_t> const& self,
                scheduler_t& scheduler)
  {
    assert(self.get() == this);
    assert(count_ > 0);

    ready_ticket_.clear();

    assert(!timeout_ticket_.empty());
    scheduler.cancel(timeout_ticket_);
    timeout_ticket_.clear();
    
    if(auto msg = context_.message_at(loglevel))
    {
      *msg << "dos_protector: " << acceptor_ << ": timeout ticket canceled";
    }

    std::unique_ptr<tcp_connection_t> incoming;
    int r = acceptor_.accept(incoming);
    assert(r == 0);
    if(incoming == nullptr)
    {
      if(auto msg = context_.message_at(loglevel))
      {
        *msg << "dos_protector: " << acceptor_ <<
          ": no incoming connection yet";
      }
    }
    else
    {
      if(auto msg = context_.message_at(loglevel))
      {
        *msg << "dos_protector: " << acceptor_ <<
          ": killing incoming connection " << *incoming;
      }
      --count_;
    }

    do_start(self, scheduler);
  }

  void on_timeout(std::shared_ptr<dos_protector_t> const& self,
                  scheduler_t& scheduler)
  {
    assert(self.get() == this);

    timeout_ticket_.clear();

    assert(!ready_ticket_.empty());
    scheduler.cancel(ready_ticket_);
    ready_ticket_.clear();

    if(auto msg = context_.message_at(loglevel))
    {
      *msg << "dos_protector: " << acceptor_ << ": ready ticket canceled";
    }

    if(auto msg = context_.message_at(loglevel))
    {
      *msg << "dos_protector: " << acceptor_ << ": timeout reached";
    }

    timed_out_ = true;
  }

private :
  logging_context_t const& context_;
  tcp_acceptor_t acceptor_;
  int count_;
  bool timed_out_;
  duration_t timeout_;
  cancellation_ticket_t ready_ticket_;
  cancellation_ticket_t timeout_ticket_;
};

void check_alarm_order(logging_context_t const& context,
                       socket_layer_t& sockets,
                       selector_factory_t const& factory)
{
  if(auto msg = context.message_at(loglevel))
  {
    *msg << "check_alarm_order(): using " << factory << " selector";
  }

  default_scheduler_t scheduler(sockets, factory);

  std::vector<int> order;

  scheduler.call_alarm(milliseconds_t(0),
    [&](stack_marker_t&) { order.push_back(0); });
  scheduler.call_alarm(milliseconds_t(1),
    [&](stack_marker_t&) { order.push_back(1); });
  scheduler.call_alarm(milliseconds_t(2),
    [&](stack_marker_t&) { order.push_back(2); });

  stack_marker_t base_marker;

  while(auto cb = scheduler.wait())
  {
    cb(base_marker);
  }

  assert(order.size() == 3);
  assert(order[0] == 0);
  assert(order[1] == 1);
  assert(order[2] == 2);
}
  
void check_alarm_order(logging_context_t const& context)
{
  socket_layer_t sockets;
  auto factories = available_selector_factories();
  for(auto const& factory : factories)
  {
    check_alarm_order(context, sockets, factory);
  }
}

void empty_scheduler(logging_context_t const& context,
                     socket_layer_t& sockets,
                     selector_factory_t const& factory)
{
  if(auto msg = context.message_at(loglevel))
  {
    *msg << "empty_scheduler(): using " << factory << " selector";
  }

  default_scheduler_t scheduler(sockets, factory);

  assert(scheduler.wait() == nullptr);
}

void empty_scheduler(logging_context_t const& context)
{
  socket_layer_t sockets;
  auto factories = available_selector_factories();
  for(auto const& factory : factories)
  {
    empty_scheduler(context, sockets, factory);
  }
}

void no_client(logging_context_t const& context,
               socket_layer_t& sockets,
               selector_factory_t const& factory,
               endpoint_t const& interface)
{
  default_scheduler_t scheduler(sockets, factory);

  auto protector = start_event_handler<dos_protector_t>(
    scheduler, context, sockets, interface, 1, milliseconds_t(1));
  endpoint_t endpoint = protector->local_endpoint();

  if(auto msg = context.message_at(loglevel))
  {
    *msg << "no_client(): using " << factory <<
      " selector; protector: " << endpoint;
  }

  assert(!protector->timed_out());

  stack_marker_t base_marker;
  while(callback_t callback = scheduler.wait())
  {
    callback(base_marker);
  }

  assert(protector->timed_out());
}

void no_client(logging_context_t const& context)
{
  socket_layer_t sockets;
  auto factories = available_selector_factories();
  auto interfaces = local_interfaces(sockets, any_port);

  for(auto const& factory : factories)
  {
    for(auto const& interface : interfaces)
    {
      no_client(context, sockets, factory, interface);
    }
  }
}

void single_client(logging_context_t const& context,
                   socket_layer_t& sockets,
                   selector_factory_t const& factory,
                   endpoint_t const& interface)
{
  default_scheduler_t scheduler(sockets, factory);

  auto protector = start_event_handler<dos_protector_t>(
    scheduler, context, sockets, interface, 1);
  endpoint_t endpoint = protector->local_endpoint();

  assert(!protector->done());

  if(auto msg = context.message_at(loglevel))
  {
    *msg << "single_client(): using " << factory <<
      " selector; protector: " << endpoint;
  }

  tcp_connection_t client(sockets, endpoint);
  if(auto msg = context.message_at(loglevel))
  {
    *msg << "single_client(): client " << client;
  }

  stack_marker_t base_marker;
  while(callback_t callback = scheduler.wait())
  {
    callback(base_marker);
  }

  assert(protector->done());
}

void single_client(logging_context_t const& context)
{
  socket_layer_t sockets;

  auto factories = available_selector_factories();
  auto interfaces = local_interfaces(sockets, any_port);

  for(auto const& factory : factories)
  {
    for(auto const& interface : interfaces)
    {
      single_client(context, sockets, factory, interface);
    }
  }
}

void multiple_clients(logging_context_t const& context,
                      socket_layer_t& sockets,
                      selector_factory_t const& factory,
                      endpoint_t const& interface)
{
  default_scheduler_t scheduler(sockets, factory);
  
  auto protector = start_event_handler<dos_protector_t>(
    scheduler, context, sockets, interface, 2);
  endpoint_t endpoint = protector->local_endpoint();

  assert(!protector->done());

  if(auto msg = context.message_at(loglevel))
  {
    *msg << "multiple_clients(): using " << factory <<
      " selector; protector: " << endpoint;
  }

  tcp_connection_t client1(sockets, endpoint);
  tcp_connection_t client2(sockets, endpoint);
  if(auto msg = context.message_at(loglevel))
  {
    *msg << "multiple_clients(): client1: " << client1 <<
      " client2: " << client2;
  }

  stack_marker_t base_marker;
  while(callback_t callback = scheduler.wait())
  {
    callback(base_marker);
  }

  assert(protector->done());
}

void multiple_clients(logging_context_t const& context)
{
  socket_layer_t sockets;
  auto factories = available_selector_factories();
  auto interfaces = local_interfaces(sockets, any_port);

  for(auto const& factory : factories)
  {
    for(auto const& interface : interfaces)
    {
      multiple_clients(context, sockets, factory, interface);
    }
  }
}

void multiple_acceptors(logging_context_t const& context,
                        socket_layer_t& sockets,
                        selector_factory_t const& factory,
                        endpoint_t const& interface)
{
  default_scheduler_t scheduler(sockets, factory);

  auto protector1 = start_event_handler<dos_protector_t>(
    scheduler, context, sockets, interface, 1);
  endpoint_t endpoint1 = protector1->local_endpoint();

  auto protector2 = start_event_handler<dos_protector_t>(
    scheduler, context, sockets, interface, 1);
  endpoint_t endpoint2 = protector2->local_endpoint();

  assert(!protector1->done());
  assert(!protector2->done());

  if(auto msg = context.message_at(loglevel))
  {
    *msg << "multiple_acceptors(): using " << factory <<
      " selector; protector1: " << endpoint1 <<
      " protector2: " << endpoint2;
  }

  tcp_connection_t client1(sockets, endpoint1);
  tcp_connection_t client2(sockets, endpoint2);
  if(auto msg = context.message_at(loglevel))
  {
    *msg << "multiple_acceptors(): client1: " << client1 <<
      " client2: " << client2;
  }

  stack_marker_t base_marker;
  while(callback_t callback = scheduler.wait())
  {
    callback(base_marker);
  }

  assert(protector1->done());
  assert(protector2->done());
}

void multiple_acceptors(logging_context_t const& context)
{
  socket_layer_t sockets;
  auto factories = available_selector_factories();
  auto interfaces = local_interfaces(sockets, any_port);

  for(auto const& factory : factories)
  {
    for(auto const& interface : interfaces)
    {
      multiple_acceptors(context, sockets, factory, interface);
    }
  }
}

void one_idle_acceptor(logging_context_t const& context,
                       socket_layer_t& sockets,
                       selector_factory_t const& factory,
                       endpoint_t const& interface)
{
  default_scheduler_t scheduler(sockets, factory);

  auto protector1 = start_event_handler<dos_protector_t>(
    scheduler, context, sockets, interface, 2);
  endpoint_t endpoint1 = protector1->local_endpoint();

  auto protector2 = start_event_handler<dos_protector_t>(
    scheduler, context, sockets, interface, 1, milliseconds_t(1));
  endpoint_t endpoint2 = protector2->local_endpoint();

  assert(!protector1->done());
  assert(!protector2->timed_out());

  if(auto msg = context.message_at(loglevel))
  {
    *msg << "one_idle_acceptor(): using " << factory <<
      " selector; protector1: " << endpoint1 <<
      " protector2: " << endpoint2;
  }

  tcp_connection_t client1(sockets, endpoint1);
  tcp_connection_t client2(sockets, endpoint1);
  if(auto msg = context.message_at(loglevel))
  {
    *msg << "one_idle_acceptor(): client1: " << client1 <<
      " client2: " << client2;
  }

  stack_marker_t base_marker;
  while(callback_t callback = scheduler.wait())
  {
    callback(base_marker);
  }

  assert(protector1->done());
  assert(protector2->timed_out());
}

void one_idle_acceptor(logging_context_t const& context)
{
  socket_layer_t sockets;
  auto factories = available_selector_factories();
  auto interfaces = local_interfaces(sockets, any_port);

  for(auto const& factory : factories)
  {
    for(auto const& interface : interfaces)
    {
      one_idle_acceptor(context, sockets, factory, interface);
    }
  }
}

void scheduler_switch(logging_context_t const& context,
                      socket_layer_t& sockets,
                      selector_factory_t const& factory,
                      endpoint_t const& interface)
{
  default_scheduler_t scheduler1(sockets, factory);
  default_scheduler_t scheduler2(sockets, factory);

  tcp_acceptor_t acceptor(sockets, interface);
  acceptor.set_nonblocking();

  // put pressure on the acceptor
  tcp_connection_t client(sockets, acceptor.local_endpoint());

  if(auto msg = context.message_at(loglevel))
  {
    *msg << "scheduler_switch(): using " << factory <<
      " selector; acceptor: " << acceptor <<
      " client: " << client;
  }

  assert(scheduler1.wait() == nullptr);
  assert(scheduler2.wait() == nullptr);

  cancellation_ticket_t ticket = acceptor.call_when_ready(
    scheduler1, [](stack_marker_t&) { });
  assert(!ticket.empty());

  assert(scheduler1.wait() != nullptr);
  ticket = acceptor.call_when_ready(scheduler1, [](stack_marker_t&) { }); 
  assert(!ticket.empty());

  assert(scheduler2.wait() == nullptr);

  scheduler1.cancel(ticket);
  ticket = acceptor.call_when_ready(scheduler2, [](stack_marker_t&) { });
  assert(!ticket.empty());

  assert(scheduler1.wait() == nullptr);
 
  assert(scheduler2.wait() != nullptr);
  ticket = acceptor.call_when_ready(scheduler2, [](stack_marker_t&) { });
  assert(!ticket.empty());

  scheduler2.cancel(ticket);
  assert(scheduler1.wait() == nullptr);
  assert(scheduler2.wait() == nullptr);
}

void scheduler_switch(logging_context_t const& context)
{
  socket_layer_t sockets;
  auto factories = available_selector_factories();
  auto interfaces = local_interfaces(sockets, any_port);

  for(auto const& factory : factories)
  {
    for(auto const& interface : interfaces)
    {
      scheduler_switch(context, sockets, factory, interface);
    }
  }
}

void run_tests(int argc, char const* const* argv)
{
  logger_t logger(std::make_unique<cuti::streambuf_backend_t>(std::cerr));
  logging_context_t context(
    logger, argc == 1 ? loglevel_t::error : loglevel_t::info);

  check_alarm_order(context);
  empty_scheduler(context);
  no_client(context);
  single_client(context);
  multiple_clients(context);
  multiple_acceptors(context);
  one_idle_acceptor(context);

  scheduler_switch(context);
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
