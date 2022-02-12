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

struct wakeup_pipe_t
{
  wakeup_pipe_t()
  : wakeup_cnt_(0)
  , pipe_reader_(nullptr)
  , pipe_writer_(nullptr)
  , readable_ticket_()
  , scheduler_(nullptr)
  , callback_()
  {
    std::tie(pipe_reader_, pipe_writer_) = make_event_pipe();
  }

  wakeup_pipe_t(wakeup_pipe_t const&) = delete;
  wakeup_pipe_t& operator=(wakeup_pipe_t const&) = delete;
  
  void wakeup()
  {
    if(wakeup_cnt_.fetch_add(1) == 0)
    {
      bool write_result = pipe_writer_->write('*');
      assert(write_result);
      static_cast<void>(write_result);
    }
  }

  bool woken_up()
  {
    unsigned int old_cnt = 1;
    do
    {
      if(wakeup_cnt_.compare_exchange_weak(old_cnt, old_cnt - 1))
      {
        break;
      }
    } while(old_cnt != 0);

    if(old_cnt == 1)
    {
      std::optional<int> read_result = pipe_reader_->read();
      assert(read_result != std::nullopt);
      assert(*read_result == '*');
      static_cast<void>(read_result);
    }

    return old_cnt != 0;
  }

  void call_when_woken_up(scheduler_t& scheduler, callback_t callback)
  {
    assert(callback != nullptr);

    this->cancel_when_woken_up();

    readable_ticket_ = pipe_reader_->call_when_readable(scheduler,
      [this] { this->on_pipe_readable(); });
    scheduler_ = &scheduler;
    callback_ = std::move(callback);
  }

  void cancel_when_woken_up()
  {
    if(!readable_ticket_.empty())
    {
      assert(scheduler_ != nullptr);
      scheduler_->cancel(readable_ticket_);
      readable_ticket_.clear();
    }

    scheduler_ = nullptr;
    callback_ = nullptr;
  }

  ~wakeup_pipe_t()
  {
    this->cancel_when_woken_up();
  }

private :
  void on_pipe_readable()
  {
    assert(!readable_ticket_.empty());
    assert(callback_ != nullptr);

    readable_ticket_.clear();
    scheduler_ = nullptr;
    callback_t callback = std::move(callback_);

    callback();
  }
      
private :
  static_assert(std::atomic<unsigned int>::is_always_lock_free);
  std::atomic<unsigned int> wakeup_cnt_;

  std::unique_ptr<event_pipe_reader_t> pipe_reader_;
  std::unique_ptr<event_pipe_writer_t> pipe_writer_;

  cancellation_ticket_t readable_ticket_;
  scheduler_t* scheduler_;
  callback_t callback_;
};

struct listener_t
{
  listener_t(logging_context_t const& context,
             endpoint_t const& endpoint,
             method_map_t const& map)
  : context_(context)
  , acceptor_(endpoint)
  , map_(map)
  , ready_ticket_()
  , scheduler_()
  , callback_()
  {
    acceptor_.set_nonblocking();

    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "listening on endpoint " << acceptor_.local_endpoint();
    }
  }

  listener_t(listener_t const&) = delete;
  listener_t& operator=(listener_t const&) = delete;
  
  endpoint_t const& endpoint() const
  {
    return acceptor_.local_endpoint();
  }

  method_map_t const& method_map() const
  {
    return map_;
  }

  std::unique_ptr<tcp_connection_t> accept()
  {
    std::unique_ptr<tcp_connection_t> accepted;

    if(auto status = acceptor_.accept(accepted))
    {
      if(auto msg = context_.message_at(loglevel_t::warning))
      {
        *msg << "failure to accept on endpoint " <<
          acceptor_.local_endpoint() << ": " << system_error_string(status);
      }
    }
      
    return accepted;
  }

  void call_when_ready(scheduler_t& scheduler, callback_t callback)
  {
    assert(callback != nullptr);

    this->cancel_when_ready();

    ready_ticket_ = acceptor_.call_when_ready(scheduler,
      [this] { this->on_acceptor_ready(); });
    scheduler_ = &scheduler;
    callback_ = std::move(callback);
  }

  void cancel_when_ready()
  {
    if(!ready_ticket_.empty())
    {
      assert(scheduler_ != nullptr);
      scheduler_->cancel(ready_ticket_);
      ready_ticket_.clear();
    }

    scheduler_ = nullptr;
    callback_ = nullptr;
  }
    
  ~listener_t()
  {
    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "closing endpoint " << acceptor_.local_endpoint();
    }

    this->cancel_when_ready();
  }

private :
  void on_acceptor_ready()
  {
    assert(!ready_ticket_.empty());
    assert(callback_ != nullptr);

    ready_ticket_.clear();
    scheduler_ = nullptr;
    callback_t callback = std::move(callback_);

    callback();
  }

private :
  logging_context_t const& context_;
  tcp_acceptor_t acceptor_;
  method_map_t const& map_;

  cancellation_ticket_t ready_ticket_;
  scheduler_t* scheduler_;
  callback_t callback_;
};

struct client_t
{
  client_t(logging_context_t const& context,
           std::unique_ptr<tcp_connection_t> conn,
           std::size_t bufsize,
           throughput_settings_t const& settings,
           method_map_t const& map)
  : context_(context)
  , nb_inbuf_()
  , nb_outbuf_()
  , settings_(settings)
  , map_(map)
  {
    assert(conn != nullptr);
    std::tie(nb_inbuf_, nb_outbuf_) =
      make_nb_tcp_buffers(std::move(conn), bufsize, bufsize);

    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "accepted connection " << *nb_inbuf_;
    }
  }

  client_t(client_t const&) = delete;
  client_t& operator=(client_t const&) = delete;

  logging_context_t const& context()
  {
    return context_;
  }

  nb_inbuf_t& nb_inbuf()
  {
    return *nb_inbuf_;
  }

  nb_outbuf_t& nb_outbuf()
  {
    return *nb_outbuf_;
  }

  throughput_settings_t const& throughput_settings() const
  {
    return settings_;
  }

  method_map_t const& method_map() const
  {
    return map_;
  }

  ~client_t()
  {
    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "closing connection " << *nb_inbuf_;
    }
  }
  
private :
  logging_context_t const& context_;
  std::unique_ptr<nb_inbuf_t> nb_inbuf_;
  std::unique_ptr<nb_outbuf_t> nb_outbuf_;
  throughput_settings_t const& settings_;
  method_map_t const& map_;
};

struct core_dispatcher_t
{
  core_dispatcher_t(logging_context_t const& context,
                    selector_factory_t const& factory,
                    std::size_t client_bufsize,
        	    throughput_settings_t settings)
  : context_(context)
  , scheduler_(factory)
  , wakeup_pipe_()
  , bufsize_(client_bufsize)
  , settings_(std::move(settings))
  , listeners_()
  , monitored_clients_()
  , other_clients_()
  , woken_up_(false)
  , selected_client_()
  {
    wakeup_pipe_.call_when_woken_up(scheduler_,
      [this] { this->on_wakeup_pipe(); });

    if(auto msg = context.message_at(loglevel_t::info))
    {
      *msg << "core dispatcher created (selector: " << factory << ')';
    }
  }

  void wakeup()
  {
    wakeup_pipe_.wakeup();
  }

  endpoint_t add_listener(endpoint_t const& endpoint, method_map_t const& map)
  {
    auto listener = listeners_.emplace(listeners_.begin(),
      context_, endpoint, map);
    listener->call_when_ready(scheduler_,
      [this, listener] { this->on_listener_ready(listener); });

    return listener->endpoint();
  }

  std::optional<std::list<client_t>::iterator> select_client()
  {
    woken_up_ = false;
    selected_client_.reset();

    while(!woken_up_ && selected_client_ == std::nullopt)
    {
      auto cb = scheduler_.wait();
      assert(cb != nullptr);
      cb();
    }

    std::optional<std::list<client_t>::iterator> result = std::nullopt;
    if(woken_up_)
    {
      assert(selected_client_ == std::nullopt);
      woken_up_ = false;
    }
    else
    {
      result = selected_client_;
      selected_client_.reset();
    }
    return result;
  }
    
  void resume_monitoring(std::list<client_t>::iterator client)
  {
    if(int status = client->nb_inbuf().error_status())
    {
      if(auto msg = context_.message_at(loglevel_t::error))
      {
        *msg << "input error on connection " << client->nb_inbuf() <<
          ": " << system_error_string(status);
      }
      other_clients_.erase(client);
    }
    else if(int status = client->nb_outbuf().error_status())
    {
      if(auto msg = context_.message_at(loglevel_t::error))
      {
        *msg << "output error on connection " << client->nb_outbuf() <<
          ": " << system_error_string(status);
      }
      other_clients_.erase(client);
    }
    else
    {
      // TODO: evict older clients based on maximum count?
      monitored_clients_.splice(monitored_clients_.begin(),
        other_clients_, client);
      client->nb_inbuf().call_when_readable(scheduler_,
        [this, client] { this->on_client_readable(client); });
    }
  }

  ~core_dispatcher_t()
  {
    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "destroying core dispatcher";
    }
  }
    
private :
  void on_wakeup_pipe()
  {
    if(wakeup_pipe_.woken_up())
    {
      woken_up_ = true;
    }

    wakeup_pipe_.call_when_woken_up(scheduler_,
      [this] { this->on_wakeup_pipe(); });
  }

  void on_listener_ready(std::list<listener_t>::iterator listener)
  {
    std::unique_ptr<tcp_connection_t> accepted;
    if((accepted = listener->accept()) != nullptr)
    {
      auto new_client = other_clients_.emplace(other_clients_.begin(),
        context_, std::move(accepted),
	bufsize_, settings_, listener->method_map());
      this->resume_monitoring(new_client);
    }

    listener->call_when_ready(scheduler_,
      [this, listener] { this->on_listener_ready(listener); });
  }
        
  void on_client_readable(std::list<client_t>::iterator client)
  {
    if(client->nb_inbuf().readable())
    {
      if(client->nb_inbuf().peek() == eof)
      {
        if(auto msg = context_.message_at(loglevel_t::info))
        {
          *msg << "end of input on connection " << client->nb_inbuf();
        }
        monitored_clients_.erase(client);
      }
      else
      {
        other_clients_.splice(other_clients_.begin(),
	  monitored_clients_, client);
        selected_client_ = client;
      }
    }
    else
    {
      client->nb_inbuf().call_when_readable(scheduler_,
        [this, client] { this->on_client_readable(client); });
    }
  }
      
private :
  logging_context_t const& context_;
  default_scheduler_t scheduler_;
  wakeup_pipe_t wakeup_pipe_;
  std::size_t const bufsize_;
  throughput_settings_t settings_;

  std::list<listener_t> listeners_;
  std::list<client_t> monitored_clients_;
  std::list<client_t> other_clients_;
  
  bool woken_up_;
  std::optional<std::list<client_t>::iterator> selected_client_;
};

void handle_request(default_scheduler_t& scheduler, client_t& client)
{
  stack_marker_t base_marker;

  bound_inbuf_t bound_inbuf(base_marker, client.nb_inbuf(), scheduler);
  bound_inbuf.enable_throughput_checking(client.throughput_settings());

  bound_outbuf_t bound_outbuf(base_marker, client.nb_outbuf(), scheduler);
  bound_outbuf.enable_throughput_checking(client.throughput_settings());

  final_result_t<void> result;
  request_handler_t request_handler(result,
    client.context(), bound_inbuf, bound_outbuf, client.method_map());
  request_handler.start();

  while(!result.available())
  {
    auto cb = scheduler.wait();
    assert(cb != nullptr);
    cb();
  }

  result.value();
}

} // anonymous

struct dispatcher_t::impl_t
{
  impl_t(logging_context_t const& context, dispatcher_config_t config)
  : context_(context)
  , config_(std::move(config))
  , core_dispatcher_(context_, config_.selector_factory_,
      config_.bufsize_, config_.throughput_settings_)
  , signal_(0)
  { }

  impl_t(impl_t const&) = delete;
  impl_t& operator=(impl_t const&) = delete;

  endpoint_t add_listener(endpoint_t const& endpoint, method_map_t const& map)
  {
    return core_dispatcher_.add_listener(endpoint, map);
  }
  
  void run()
  {
    default_scheduler_t request_scheduler;

    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "dispatcher running";
    }

    std::optional<std::list<client_t>::iterator> client;
    while((client = core_dispatcher_.select_client()) != std::nullopt)
    {
      handle_request(request_scheduler, **client);
      core_dispatcher_.resume_monitoring(*client);
    }

    auto sig = signal_.load(std::memory_order_acquire);
    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "caught signal " << sig << ", stopping dispatcher";
    }
  }

  void stop(int sig)
  {
    signal_.store(sig, std::memory_order_release);
    core_dispatcher_.wakeup();
  }

private :
  logging_context_t const& context_;
  dispatcher_config_t config_;
  core_dispatcher_t core_dispatcher_;

  static_assert(std::atomic<int>::is_always_lock_free);
  std::atomic<int> signal_;
};

dispatcher_t::dispatcher_t(logging_context_t const& context,
                           dispatcher_config_t config)
: impl_(std::make_unique<impl_t>(context, std::move(config)))
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
