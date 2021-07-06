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

#include <cuti/default_scheduler.hpp>
#include <cuti/resolver.hpp>
#include <cuti/tcp_acceptor.hpp>
#include <cuti/tcp_connection.hpp>
#include <cuti/ticket_holder.hpp>

#include <tuple>

// enable assert()
#undef NDEBUG
#include <cassert>

namespace /* anonymous */
{

using namespace cuti;

void empty_holder()
{
  ticket_holder_t holder;
  assert(holder.empty());
}

void alarm_at()
{
  default_scheduler_t scheduler;
  ticket_holder_t holder;

  bool called = false;

  holder.call_alarm(scheduler, cuti_clock_t::now(), [&] { called = true; });
  assert(!holder.empty());

  while(auto callback = scheduler.wait())
  {
    callback();
  }

  assert(called);
  assert(holder.empty());
}
  
void alarm_in()
{
  default_scheduler_t scheduler;
  ticket_holder_t holder;

  bool called = false;

  holder.call_alarm(scheduler, duration_t::zero(), [&] { called = true; });
  assert(!holder.empty());

  while(auto callback = scheduler.wait())
  {
    callback();
  }

  assert(called);
  assert(holder.empty());
}
  
void when_ready()
{
  default_scheduler_t scheduler;

  tcp_acceptor_t acceptor(local_interfaces(0).front());
  acceptor.set_nonblocking();
  tcp_connection_t connection(acceptor.local_endpoint());

  ticket_holder_t holder;

  bool called = false;

  holder.call_when_ready(scheduler, acceptor, [&] { called = true; });
  assert(!holder.empty());

  while(auto callback = scheduler.wait())
  {
    callback();
  }

  assert(called);
  assert(holder.empty());
}

void when_writable()
{
  default_scheduler_t scheduler;

  std::unique_ptr<tcp_connection_t> conn_out;
  std::unique_ptr<tcp_connection_t> conn_in;
  std::tie(conn_out, conn_in) = make_connected_pair();
  conn_out->set_nonblocking();

  ticket_holder_t holder;

  bool called = false;

  holder.call_when_writable(scheduler, *conn_out, [&] { called = true; });
  assert(!holder.empty());

  while(auto callback = scheduler.wait())
  {
    callback();
  }

  assert(called);
  assert(holder.empty());
}

void when_readable()
{
  default_scheduler_t scheduler;

  std::unique_ptr<tcp_connection_t> conn_out;
  std::unique_ptr<tcp_connection_t> conn_in;
  std::tie(conn_out, conn_in) = make_connected_pair();

  conn_out->close_write_end();
  conn_in->set_nonblocking();

  ticket_holder_t holder;

  bool called = false;

  holder.call_when_readable(scheduler, *conn_in, [&] { called = true; });
  assert(!holder.empty());

  while(auto callback = scheduler.wait())
  {
    callback();
  }

  assert(called);
  assert(holder.empty());
}

void cancel()
{
  default_scheduler_t scheduler;
  ticket_holder_t holder;

  holder.call_alarm(scheduler, cuti_clock_t::now(), [] {});
  assert(!holder.empty());

  holder.cancel();

  assert(scheduler.wait() == nullptr);
  assert(holder.empty());
}
  
void out_of_scope()
{
  default_scheduler_t scheduler;

  {
    ticket_holder_t holder;

    holder.call_alarm(scheduler, cuti_clock_t::now(), [] {});
    assert(!holder.empty());
  }

  assert(scheduler.wait() == nullptr);
}

void change_scheduler()
{
  default_scheduler_t final_scheduler;
  ticket_holder_t holder;

  bool called = false;
  {
    default_scheduler_t initial_scheduler;

    holder.call_alarm(initial_scheduler, cuti_clock_t::now(),
      [&] { called = true; });
    assert(!holder.empty());

    holder.call_alarm(final_scheduler, cuti_clock_t::now(),
      [&] { called = true; });
    assert(!holder.empty());

    assert(initial_scheduler.wait() == nullptr);
  }

  while(auto callback = final_scheduler.wait())
  {
    callback();
  }

  assert(called);
  assert(holder.empty());
}
  
} // anonymous

int main()
{
  empty_holder();

  alarm_at();
  alarm_in();

  when_ready();
  when_writable();
  when_readable();
  
  cancel();
  out_of_scope();
  change_scheduler();

  return 0;
}
