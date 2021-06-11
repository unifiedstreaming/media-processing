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

#include "dispatcher.hpp"

#include "default_scheduler.hpp"
#include "logging_context.hpp"
#include "tcp_connection.hpp"

#include <utility>

namespace cuti
{

namespace // anonymous
{

void on_control(int& sig,
                tcp_connection_t& connection,
		scheduler_t& scheduler)
{
  char buf[1];
  char* next;
  connection.read(buf, buf + 1, next);

  if(next == nullptr)
  {
    // spurious callback
  }
  else if(next == buf)
  {
    throw system_exception_t("unexpected EOF on control connection");
  }
  else
  {
    assert(next == buf + 1);
    sig = buf[0];
    assert(sig != 0);
  }

  connection.call_when_readable(scheduler,
    [&] { on_control(sig, connection, scheduler); });
}

} // anonymous

dispatcher_t::dispatcher_t(logging_context_t& logging_context,
                           tcp_connection_t& control_connection,
                           selector_factory_t selector_factory)
: logging_context_(logging_context)
, control_connection_(control_connection)
, selector_factory_(std::move(selector_factory))
{ }

void dispatcher_t::run()
{
  default_scheduler_t scheduler(selector_factory_);

  int sig = 0;
  control_connection_.call_when_readable(scheduler,
    [&] { on_control(sig, control_connection_, scheduler); } );

  if(auto msg = logging_context_.message_at(loglevel_t::info))
  {
    *msg << "dispatcher running (selector: " << selector_factory_ << ")";
  }

  while(sig == 0)
  {
    auto callback = scheduler.wait();
    assert(callback != nullptr);
    callback();
  }

  if(auto msg = logging_context_.message_at(loglevel_t::info))
  {
    *msg << "caught signal " << sig << ", stopping dispatcher";
  }
}

} // cuti
