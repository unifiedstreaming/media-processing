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

#include "async_readers.hpp"
#include "bound_inbuf.hpp"
#include "bound_outbuf.hpp"
#include "default_scheduler.hpp"
#include "event_pipe.hpp"
#include "final_result.hpp"
#include "logging_context.hpp"
#include "method_map.hpp"
#include "nb_tcp_buffers.hpp"
#include "request_handler.hpp"
#include "system_error.hpp"
#include "tcp_acceptor.hpp"
#include "tcp_connection.hpp"

#include <cassert>
#include <limits>
#include <list>
#include <string>
#include <tuple>
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
           method_map_t const& map,
           dispatcher_config_t const& config)
  : context_(context)
  , nb_inbuf_()
  , nb_outbuf_()
  , map_(map)
  , config_(config)
  {
    std::tie(nb_inbuf_, nb_outbuf_) =
      make_nb_tcp_buffers(std::move(connection));

    if(auto msg = context.message_at(loglevel_t::info))
    {
      *msg << "accepted client " << *nb_inbuf_;
    }
  }

  void call_when_readable(
    scheduler_t& scheduler, callback_t callback)
  {
    nb_inbuf_->call_when_readable(scheduler, std::move(callback));
  }

  bool on_readable()
  {
    default_scheduler_t scheduler;
    stack_marker_t base_marker;

    bound_inbuf_t bound_inbuf(base_marker, *nb_inbuf_, scheduler);
    bound_inbuf.enable_throughput_checking(
      config_.min_bytes_per_tick_,
      config_.low_ticks_limit_,
      config_.tick_length_);

    bound_outbuf_t bound_outbuf(base_marker, *nb_outbuf_, scheduler);
    bound_outbuf.enable_throughput_checking(
      config_.min_bytes_per_tick_,
      config_.low_ticks_limit_,
      config_.tick_length_);

    final_result_t<bool> at_eof_result;
    eof_checker_t eof_checker(at_eof_result, bound_inbuf);
    eof_checker.start();

    while(!at_eof_result.available())
    {
      auto cb = scheduler.wait();
      assert(cb != nullptr);
      cb();
    }

    if(at_eof_result.value())
    {
      if(int status = bound_inbuf.error_status())
      {
        if(auto msg = context_.message_at(loglevel_t::error))
        {
          *msg << "client " << bound_inbuf << ": input error: " <<
            system_error_string(status);
        }
        return false;
      }

      if(auto msg = context_.message_at(loglevel_t::info))
      {
        *msg << "client " << bound_inbuf << ": end of input";
      }
      return false;
    }

    final_result_t<void> request_handler_result;
    request_handler_t request_handler(
      request_handler_result, context_, bound_inbuf, bound_outbuf, map_);
    request_handler.start();

    while(!request_handler_result.available())
    {
      auto cb = scheduler.wait();
      assert(cb != nullptr);
      cb();
    }

    request_handler_result.value();

    if(int status = bound_inbuf.error_status())
    {
      if(auto msg = context_.message_at(loglevel_t::error))
      {
        *msg << "client " << bound_inbuf << ": input error: " <<
          system_error_string(status);
      }
      return false;
    }

    if(int status = bound_outbuf.error_status())
    {
      if(auto msg = context_.message_at(loglevel_t::error))
      {
        *msg << "client " << bound_outbuf << ": output error: " <<
          system_error_string(status);
      }
      return false;
    }

    return true;
  }

  ~client_t()
  {
    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "disconnecting client " << *nb_inbuf_;
    }
  }

private :
  logging_context_t const& context_;
  std::unique_ptr<nb_inbuf_t> nb_inbuf_;
  std::unique_ptr<nb_outbuf_t> nb_outbuf_;
  method_map_t const& map_;
  dispatcher_config_t const& config_;
};

struct listener_t
{
  listener_t(logging_context_t const& context,
             endpoint_t const& endpoint,
             method_map_t const& map,
             dispatcher_config_t const& config)
  : context_(context)
  , acceptor_(std::make_unique<tcp_acceptor_t>(endpoint))
  , map_(map)
  , config_(config)
  {
    acceptor_->set_nonblocking();

    if(auto msg = context.message_at(loglevel_t::info))
    {
        *msg << "listening at endpoint " << *acceptor_;
    }
  }

  listener_t(listener_t const&) = delete;
  listener_t& operator=(listener_t const&) = delete;

  endpoint_t const& endpoint() const
  {
    return acceptor_->local_endpoint();
  }

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
      result = std::make_unique<client_t>(
        context_, std::move(accepted), map_, config_);
    }

    return result;
  }

private :
  logging_context_t const& context_;
  std::unique_ptr<tcp_acceptor_t> acceptor_;
  method_map_t const& map_;
  dispatcher_config_t const& config_;
};

} // anonymous

struct dispatcher_t::impl_t
{
  impl_t(logging_context_t const& logging_context,
         dispatcher_config_t config)
  : logging_context_(logging_context)
  , config_(std::move(config))
  , sig_(0)
  , stop_reader_()
  , stop_writer_()
  , listeners_()
  , scheduler_(config_.selector_factory_)
  , clients_()
  {
    std::tie(stop_reader_, stop_writer_) = make_event_pipe();
    stop_reader_->set_nonblocking();
    stop_writer_->set_nonblocking();
    stop_reader_->call_when_readable(scheduler_,
      [this] { this->on_stop_reader(); });
  }

  impl_t(impl_t const&) = delete;
  impl_t& operator=(impl_t const&) = delete;

  endpoint_t add_listener(endpoint_t const& endpoint, method_map_t const& map)
  {
    listeners_.push_back(
      std::make_unique<listener_t>(logging_context_, endpoint, map, config_));
    scoped_guard_t guard([&] { listeners_.pop_back(); });

    listener_t& last = *listeners_.back();
    last.call_when_ready(
      scheduler_, [this, &last ] { this->on_listener(last); } );
  
    guard.dismiss();

    return last.endpoint();
  }
  
  void run()
  {
    if(auto msg = logging_context_.message_at(loglevel_t::info))
    {
      *msg << "dispatcher running (selector: " <<
        config_.selector_factory_.name() << ")";
    }

    sig_ = std::nullopt;
    do
    {
      auto callback = scheduler_.wait();
      assert(callback != nullptr);
      callback();
    } while(sig_ == std::nullopt);

    if(auto msg = logging_context_.message_at(loglevel_t::info))
    {
      *msg << "caught signal " << *sig_ << ", stopping dispatcher";
    }
  }

  void stop(int sig)
  {
    stop_writer_->write(static_cast<unsigned char>(sig));
  }

private :
  void on_stop_reader()
  {
    sig_ = stop_reader_->read();
    stop_reader_->call_when_readable(scheduler_,
      [this] { this->on_stop_reader(); });
  }

  void on_listener(listener_t& listener)
  {
    auto client = listener.on_ready();
    if(client != nullptr)
    {
      auto pos = clients_.insert(clients_.end(), std::move(client));
      (*pos)->call_when_readable(
        scheduler_, [this, pos] { this->on_client(pos); });
    }
  
    listener.call_when_ready(
      scheduler_, [this, &listener] { this->on_listener(listener); } );
  }

  void on_client(std::list<std::unique_ptr<client_t>>::iterator pos)
  {
    bool more = (*pos)->on_readable();
    if(more)
    {
      (*pos)->call_when_readable(
        scheduler_, [this, pos] { this->on_client(pos); });
    }
    else
    {
      clients_.erase(pos);
    }
  }

private :
  logging_context_t const& logging_context_;
  dispatcher_config_t config_;
  std::optional<int> sig_;
  std::unique_ptr<event_pipe_reader_t> stop_reader_;
  std::unique_ptr<event_pipe_writer_t> stop_writer_;
  std::vector<std::unique_ptr<listener_t>> listeners_;
  default_scheduler_t scheduler_;
  std::list<std::unique_ptr<client_t>> clients_;
};

dispatcher_t::dispatcher_t(logging_context_t const& logging_context,
                           dispatcher_config_t config)
: impl_(std::make_unique<impl_t>(logging_context, std::move(config)))
{ }

endpoint_t dispatcher_t::add_listener(
  endpoint_t const& endpoint, method_map_t const& map)
{
  return impl_->add_listener(endpoint, map);
}

void dispatcher_t::run()
{
  impl_->run();
}

void dispatcher_t::stop(int sig)
{
  impl_->stop(sig);
}

dispatcher_t::~dispatcher_t()
{ }

} // cuti
