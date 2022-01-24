/*
 * Copyright (C) 2021-2022 CodeShop B.V.
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
#include "event_pipe.hpp"
#include "logging_context.hpp"
#include "method_map.hpp"
#include "tcp_acceptor.hpp"
#include "tcp_connection.hpp"

#include <string>
#include <utility>
#include <vector>

namespace cuti
{

namespace // anonymous
{

struct client_t
{
  client_t(logging_context_t const& context,
           std::unique_ptr<tcp_connection_t> connection,
           method_map_t const& map)
  : context_(context)
  , connection_(std::move(connection))
  // , map_(map)
  {
    connection_->set_nonblocking();
    if(auto msg = context.message_at(loglevel_t::info))
    {
      *msg << "accepted client " << *connection_;
    }
  }

  cancellation_ticket_t call_when_readable(
    scheduler_t& scheduler, callback_t callback)
  {
    return connection_->call_when_readable(scheduler, std::move(callback));
  }

  bool on_readable()
  {
    return false;
  }

  ~client_t()
  {
    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "disconnecting client " << *connection_;
    }
  }

private :
  logging_context_t const& context_;
  std::unique_ptr<tcp_connection_t> connection_;
  // method_map_t const& map_;
};

struct listener_t
{
  listener_t(logging_context_t const& context,
             endpoint_t const& endpoint,
             method_map_t const& map)
  : context_(context)
  , acceptor_(std::make_unique<tcp_acceptor_t>(endpoint))
  , map_(map)
  {
    acceptor_->set_nonblocking();

    if(auto msg = context.message_at(loglevel_t::info))
    {
        *msg << "listening at endpoint " << *acceptor_;
    }
  }

  listener_t(listener_t const&) = delete;
  listener_t& operator=(listener_t const&) = delete;

  cancellation_ticket_t call_when_ready(
    scheduler_t& scheduler, callback_t callback)
  {
    return acceptor_->call_when_ready(scheduler, std::move(callback));
  }

  std::unique_ptr<client_t> on_ready()
  {
    std::unique_ptr<tcp_connection_t> accepted;
    std::unique_ptr<client_t> result;

    int error = acceptor_->accept(accepted);
    if(error != 0)
    {
      if(auto msg = context_.message_at(loglevel_t::error))
      {
        *msg << "listener " << *acceptor_ << ": accept() failure: " <<
          system_error_string(error);
      }
    }
    else if(accepted == nullptr)
    {
      if(auto msg = context_.message_at(loglevel_t::warning))
      {
        *msg << "listener " << *acceptor_ << ": accept() would block";
      }
    }
    else
    {
      result = std::make_unique<client_t>(context_, std::move(accepted), map_);
    }

    return result;
  }

private :
  logging_context_t const& context_;
  std::unique_ptr<tcp_acceptor_t> acceptor_;
  method_map_t const& map_;
};

} // anonymous

struct dispatcher_t::impl_t
{
  impl_t(logging_context_t const& logging_context,
         event_pipe_reader_t& control,
         selector_factory_t const& selector_factory)
  : logging_context_(logging_context)
  , control_(control)
  , listeners_()
  , selector_name_(selector_factory.name())
  , scheduler_(selector_factory)
  , sig_(0)
  {
    auto callback = [this] { this->on_control(); };
    control_.call_when_readable(scheduler_, callback);
  }

  impl_t(impl_t const&) = delete;
  impl_t& operator=(impl_t const&) = delete;

  void add_listener(endpoint_t const& endpoint, method_map_t const& map)
  {
    listeners_.push_back(
      std::make_unique<listener_t>(logging_context_, endpoint, map));
    scoped_guard_t guard([&] { listeners_.pop_back(); });

    listener_t& last = *listeners_.back();
    auto callback = [this, &last ] { this->on_listener(last); };
    last.call_when_ready(scheduler_, callback);
  
    guard.dismiss();
  }
  
  void run()
  {
    if(auto msg = logging_context_.message_at(loglevel_t::info))
    {
      *msg << "dispatcher running (selector: " << selector_name_ << ")";
    }

    sig_ = 0;
    do
    {
      auto callback = scheduler_.wait();
      assert(callback != nullptr);
      callback();
    } while(sig_ == 0);

    if(auto msg = logging_context_.message_at(loglevel_t::info))
    {
      *msg << "caught signal " << sig_ << ", stopping dispatcher";
    }
  }

private :
  void on_control()
  {
    std::optional<int> rr = control_.read();

    if(rr == std::nullopt)
    {
      // spurious callback
    }
    else if(*rr == eof)
    {
      throw system_exception_t("unexpected EOF on control pipe");
    }
    else
    {
      sig_ = *rr;
      assert(sig_ != 0);
    }

    auto callback = [this] { this->on_control(); };
    control_.call_when_readable(scheduler_, callback);
  }

  void on_listener(listener_t& listener)
  {
    std::unique_ptr<client_t> client = listener.on_ready();
    // TODO: have scheduler monitor client
    client.reset();
  
    auto callback = [this, &listener] { this->on_listener(listener); };
    listener.call_when_ready(scheduler_, callback);
  }

private :
  logging_context_t const& logging_context_;
  event_pipe_reader_t& control_;
  std::vector<std::unique_ptr<listener_t>> listeners_;
  std::string selector_name_;
  default_scheduler_t scheduler_;
  int sig_;
};

dispatcher_t::dispatcher_t(logging_context_t const& logging_context,
                           event_pipe_reader_t& control,
                           selector_factory_t const& selector_factory)
: impl_(std::make_unique<impl_t>(logging_context, control, selector_factory))
{ }

void dispatcher_t::add_listener(
  endpoint_t const& endpoint, method_map_t const& map)
{
  impl_->add_listener(endpoint, map);
}

void dispatcher_t::run()
{
  impl_->run();
}

dispatcher_t::~dispatcher_t()
{ }

} // cuti
