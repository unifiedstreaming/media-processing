/*
 * Copyright (C) 2021-2023 CodeShop B.V.
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
#include "error_status.hpp"
#include "event_pipe.hpp"
#include "final_result.hpp"
#include "logging_context.hpp"
#include "method_map.hpp"
#include "nb_tcp_buffers.hpp"
#include "request_handler.hpp"
#include "scoped_thread.hpp"
#include "tcp_acceptor.hpp"
#include "tcp_connection.hpp"

#include <cassert>
#include <condition_variable>
#include <limits>
#include <list>
#include <mutex>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace cuti
{

namespace // anonymous
{

struct wakeup_flag_t
{
  wakeup_flag_t()
  : counter_(0)
  , pipe_reader_(nullptr)
  , pipe_writer_(nullptr)
  , readable_ticket_()
  , scheduler_(nullptr)
  , callback_()
  {
    std::tie(pipe_reader_, pipe_writer_) = make_event_pipe();
  }

  wakeup_flag_t(wakeup_flag_t const&) = delete;
  wakeup_flag_t& operator=(wakeup_flag_t const&) = delete;
  
  /*
   * This function is signal- and thread-safe.
   */
  void raise()
  {
    if(counter_.fetch_add(1) == 0)
    {
      bool write_result = pipe_writer_->write('*');
      assert(write_result);
      static_cast<void>(write_result);
    }
  }

  /*
   * This function is signal- and thread-safe.
   */
  bool is_up() const
  {
    return counter_.load(std::memory_order_acquire) != 0;
  }

  /*
   * This function is signal- and thread-safe.
   */
  bool lower()
  {
    unsigned int old_counter = 1;
    do
    {
      if(counter_.compare_exchange_weak(old_counter, old_counter - 1))
      {
        break;
      }
    } while(old_counter != 0);

    if(old_counter == 1)
    {
      std::optional<int> read_result = pipe_reader_->read();
      assert(read_result.has_value());
      assert(*read_result == '*');
      static_cast<void>(read_result);
    }

    return old_counter != 0;
  }

  void call_when_up(scheduler_t& scheduler, callback_t callback)
  {
    assert(callback != nullptr);

    this->cancel_when_up();

    readable_ticket_ = pipe_reader_->call_when_readable(scheduler,
      [this] { this->on_pipe_readable(); });
    scheduler_ = &scheduler;
    callback_ = std::move(callback);
  }

  void cancel_when_up()
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

  ~wakeup_flag_t()
  {
    this->cancel_when_up();
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
  std::atomic<unsigned int> counter_;

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
          acceptor_.local_endpoint() << ": " << error_status_t(status);
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
                    dispatcher_config_t const& config)
  : context_(context)
  , config_(config)
  , scheduler_(config_.selector_factory_)
  , wakeup_flag_()
  , listeners_()
  , monitored_clients_()
  , served_clients_()
  , woken_up_(false)
  , selected_client_()
  {
    wakeup_flag_.call_when_up(scheduler_,
      [this] { this->on_wakeup_flag(); });

    if(auto msg = context.message_at(loglevel_t::info))
    {
      *msg << "dispatcher created (selector: " <<
        config_.selector_factory_ << ')';
    }
  }

  /*
   * This function is signal- and thread-safe.
   */
  void raise_wakeup_flag()
  {
    wakeup_flag_.raise();
  }

  /*
   * This function is signal- and thread-safe.
   */
  bool lower_wakeup_flag()
  {
    return wakeup_flag_.lower();
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
    assert(!woken_up_);
    assert(!selected_client_.has_value());

    do 
    {
      auto cb = scheduler_.wait();
      assert(cb != nullptr);
      cb();
    } while(!woken_up_ && !selected_client_.has_value());

    if(woken_up_)
    {
      assert(!selected_client_.has_value());
      woken_up_ = false;
      return std::nullopt;
    }

    auto result = selected_client_;
    selected_client_.reset();
    return result;
  }
    
  void resume_monitoring(std::list<client_t>::iterator client,
                         bool handler_completed)
  {
    if(!handler_completed)
    {
      if(auto msg = context_.message_at(loglevel_t::error))
      {
        *msg << "request handling on connection " << client->nb_inbuf() <<
          " interrupted";
      }
      served_clients_.erase(client);
    }
    else if(auto status = client->nb_inbuf().error_status())
    {
      if(auto msg = context_.message_at(loglevel_t::error))
      {
        *msg << "input error on connection " << client->nb_inbuf() <<
          ": " << status;
      }
      served_clients_.erase(client);
    }
    else if(auto status = client->nb_outbuf().error_status())
    {
      if(auto msg = context_.message_at(loglevel_t::error))
      {
        *msg << "output error on connection " << client->nb_outbuf() <<
          ": " << status;
      }
      served_clients_.erase(client);
    }
    else
    {
      if(config_.max_connections_ != 0 &&
         monitored_clients_.size() == config_.max_connections_)
      {
        auto oldest_client = monitored_clients_.end();
        --oldest_client;
        if(auto msg = context_.message_at(loglevel_t::error))
        {
          *msg << "maximum number of connections (" <<
            config_.max_connections_ <<
            ") exceeded; evicting least recently active connection " <<
            oldest_client->nb_inbuf();
        }
        monitored_clients_.erase(oldest_client);
      }

      monitored_clients_.splice(monitored_clients_.begin(),
        served_clients_, client);
      client->nb_inbuf().call_when_readable(scheduler_,
        [this, client] { this->on_client_readable(client); });
    }
  }

  ~core_dispatcher_t()
  {
    served_clients_.clear();
    monitored_clients_.clear();
    listeners_.clear();

    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "dispatcher destroyed";
    }
  }
    
private :
  void on_wakeup_flag()
  {
    if(wakeup_flag_.is_up())
    {
      woken_up_ = true;
    }

    wakeup_flag_.call_when_up(scheduler_,
      [this] { this->on_wakeup_flag(); });
  }

  void on_listener_ready(std::list<listener_t>::iterator listener)
  {
    std::unique_ptr<tcp_connection_t> accepted;
    if((accepted = listener->accept()) != nullptr)
    {
      auto new_client = served_clients_.emplace(served_clients_.begin(),
        context_, std::move(accepted),
        config_.bufsize_, config_.throughput_settings_,
        listener->method_map());

      bool handler_completed = true;
      this->resume_monitoring(new_client, handler_completed);
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
        served_clients_.splice(served_clients_.begin(),
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
  dispatcher_config_t const& config_;
  default_scheduler_t scheduler_;
  wakeup_flag_t wakeup_flag_;

  std::list<listener_t> listeners_;

  // Each client is either being monitored or being served 
  std::list<client_t> monitored_clients_;
  std::list<client_t> served_clients_;
  
  bool woken_up_;
  std::optional<std::list<client_t>::iterator> selected_client_;
};

struct core_mutex_t
{
  explicit core_mutex_t(core_dispatcher_t& core)
  : core_(core)
  , internal_mutex_()
  , n_urgent_waiters_(0)
  , locked_(false)
  , unlocked_with_urgent_waiters_()
  , unlocked_without_urgent_waiters_()
  { }

  core_mutex_t(core_mutex_t const&) = delete;
  core_mutex_t& operator=(core_mutex_t const&) = delete;

  /*
   * This function is thread-safe.
   */
  void urgent_lock()
  {
    std::unique_lock<std::mutex> internal_lock(internal_mutex_);

    if(locked_)
    {
      ++n_urgent_waiters_;
      core_.raise_wakeup_flag();

      do
      {
        unlocked_with_urgent_waiters_.wait(internal_lock);
      } while(locked_);

      bool was_up = core_.lower_wakeup_flag();
      assert(was_up);
      static_cast<void>(was_up);

      --n_urgent_waiters_;
    }

    locked_ = true;
  }

  /*
   * This function is thread-safe.
   */
  void normal_lock()
  {
    std::unique_lock<std::mutex> internal_lock(internal_mutex_);

    while(locked_ || n_urgent_waiters_ != 0)
    {
      unlocked_without_urgent_waiters_.wait(internal_lock);
    }

    locked_ = true;
  }

  /*
   * This function is thread-safe.
   */
  void unlock()
  {
    bool had_urgent_waiters;
    {
      std::unique_lock<std::mutex> internal_lock(internal_mutex_);

      had_urgent_waiters = n_urgent_waiters_ != 0;

      assert(locked_);
      locked_ = false;
    }

    if(had_urgent_waiters)
    {
      unlocked_with_urgent_waiters_.notify_one();
    }
    else
    {
      unlocked_without_urgent_waiters_.notify_one();
    }
  }
    
private :
  core_dispatcher_t& core_;
  mutable std::mutex internal_mutex_;
  unsigned int n_urgent_waiters_;
  bool locked_;
  std::condition_variable unlocked_with_urgent_waiters_;
  std::condition_variable unlocked_without_urgent_waiters_;
};

struct core_lock_t
{
  core_lock_t(core_mutex_t& mutex, bool urgent)
  : mutex_(mutex)
  {
    if(urgent)
    {
      mutex_.urgent_lock();
    }
    else
    {
      mutex_.normal_lock();
    }
  }

  core_lock_t(core_lock_t const&) = delete;
  core_lock_t& operator=(core_lock_t const&) = delete;
  
  ~core_lock_t()
  {
    mutex_.unlock();
  }

private :
  core_mutex_t& mutex_;
};

struct thread_pool_t;

struct pooled_thread_t
{
  template<typename F>
  pooled_thread_t(logging_context_t const& context,
                  thread_pool_t& pool,
                  std::size_t id,
                  F&& f)
  : context_(context)
  , pool_(pool)
  , id_(id)
  , interrupted_(false)
  , scheduler_()
  , wakeup_flag_()
  , mutex_()
  , joined_(false)
  , just_joined_()
  , thread_()
  {
    wakeup_flag_.call_when_up(scheduler_,
      [this] { this->on_wakeup_flag(); });
    thread_.emplace(
      [this, f = std::forward<F>(f) ] { this->run(f); });
  }

  pooled_thread_t(pooled_thread_t const&) = delete;
  pooled_thread_t& operator=(pooled_thread_t const&) = delete;

  thread_pool_t& pool() const
  {
    return pool_;
  }

  std::size_t id() const
  {
    return id_;
  }

  bool interrupted() const
  {
    return interrupted_;
  }
  
  default_scheduler_t& scheduler()
  {
    return scheduler_;
  }

  /*
   * This function is thread-safe.
   */
  void join()
  {
    std::unique_lock lock(mutex_);

    if(!joined_)
    {
      wakeup_flag_.raise();
      do
      {
        just_joined_.wait(lock);
      } while(!joined_);
    }
  }
      
  ~pooled_thread_t()
  {
    this->join();
  }

private :
  void on_wakeup_flag()
  {
    bool was_up = wakeup_flag_.lower();
    if(was_up)
    {
      interrupted_ = true;
    }
    else
    {
      wakeup_flag_.call_when_up(scheduler_,
        [this] { this->on_wakeup_flag(); });
    }
  }
      
  template<typename F>
  void run(F& f)
  {
    try
    {
      f(*this);
    }
    catch(std::exception const& ex)
    {
      if(auto msg = context_.message_at(loglevel_t::error))
      {
        *msg << "FATAL: exception in dispatcher thread " <<
          id_ << ": " << ex.what();
      }
      std::abort();
    }
    catch(...)
    {
      if(auto msg = context_.message_at(loglevel_t::error))
      {
        *msg << "FATAL: exception of unknown type in dispatcher thread " <<
          id_;
      }
      std::abort();
    }
    
    {
      std::unique_lock<std::mutex> lock(mutex_);
      joined_ = true;
    }
    just_joined_.notify_all();
  }
      
private :
  logging_context_t const& context_;
  thread_pool_t& pool_;
  std::size_t const id_;
  bool interrupted_;
  default_scheduler_t scheduler_;
  wakeup_flag_t wakeup_flag_;

  std::mutex mutex_;
  bool joined_;
  std::condition_variable just_joined_;
  std::optional<scoped_thread_t> thread_;
};

struct thread_pool_t
{
  thread_pool_t(logging_context_t const& context, std::size_t max_size)
  : context_(context)
  , max_size_(max_size)
  , mutex_()
  , frozen_(false)
  , threads_()
  { }

  thread_pool_t(thread_pool_t const&) = delete;
  thread_pool_t& operator=(thread_pool_t const&) = delete;
  
  template<typename F>
  bool add(F&& f)
  {
    std::unique_lock<std::mutex> lock(mutex_);

    if(frozen_ || (max_size_ != 0 && threads_.size() == max_size_))
    {
      return false;
    }

    threads_.emplace_back(
      context_, *this, threads_.size(), std::forward<F>(f));

    if(max_size_ > 1 && threads_.size() == max_size_)
    {
      if(auto msg = context_.message_at(loglevel_t::warning))
      {
        *msg << "maximum thread pool size (" << max_size_ <<
          ") reached; concurrent requests may be delayed";
      }
    }

    return true;
  }

  /*
   * This function is thread-safe.
   */
  void join()
  {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      frozen_ = true;
    }

    for(auto& thread : threads_)
    {
      thread.join();
    }
  }

  ~thread_pool_t()
  {
    this->join();
  }
      
private :
  logging_context_t const& context_;
  std::size_t const max_size_;
  std::mutex mutex_;
  bool frozen_;
  std::list<pooled_thread_t> threads_;
};

/*
 * Handle a request using current_thread's scheduler.  Returns true if
 * the request handler completed, and false if the handler was
 * interrupted.  An interrupted request leaves the client in some
 * unspecified intermediate state; it is then up to the caller to
 * force a remote protocol error by destroying the client.
 */
bool handle_request(pooled_thread_t& current_thread, client_t& client)
{
  default_scheduler_t& scheduler = current_thread.scheduler();

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
    if(current_thread.interrupted())
    {
      return false;
    }

    auto cb = scheduler.wait();
    assert(cb != nullptr);
    cb();
  }

  result.value();
  return true;
}

} // anonymous

struct dispatcher_t::impl_t
{
  impl_t(logging_context_t const& context, dispatcher_config_t config)
  : context_(context)
  , config_(std::move(config))
  , core_(context_, config_)
  , n_idle_threads_(0)
  , mutex_(core_)
  , dispatcher_stopping_(false)
  , signal_reader_()
  , signal_writer_()
  {
    std::tie(signal_reader_, signal_writer_) = make_event_pipe();
    signal_writer_->set_nonblocking();
  }

  impl_t(impl_t const&) = delete;
  impl_t& operator=(impl_t const&) = delete;

  endpoint_t add_listener(endpoint_t const& endpoint, method_map_t const& map)
  {
    return core_.add_listener(endpoint, map);
  }
  
  void run()
  {
    thread_pool_t thread_pool(context_, config_.max_thread_pool_size_);

    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "dispatcher running";
    }

    assert(n_idle_threads_ == 0);
    n_idle_threads_ = 1;

    bool thread_started = thread_pool.add(
      [this](pooled_thread_t& added) { this->serve(added); });
    assert(thread_started);
    static_cast<void>(thread_started);

    std::optional<int> sig = signal_reader_->read();
    assert(sig.has_value());
    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "caught signal " << *sig << ", stopping dispatcher";
    }

    dispatcher_stopping_ = true;
    core_.raise_wakeup_flag();

    thread_pool.join();
    assert(n_idle_threads_ == 0);

    bool flag_was_up = core_.lower_wakeup_flag();
    assert(flag_was_up);
    static_cast<void>(flag_was_up);

    dispatcher_stopping_ = false;

    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "dispatcher stopped";
    }
  }
  
  /*
   * This function is signal- and thread-safe.
   */
  void stop(int sig)
  {
    signal_writer_->write(static_cast<unsigned char>(sig));
  }
  
private :
  void serve(pooled_thread_t& current_thread)
  {
    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "dispatcher thread " << current_thread.id() << " started";
    }

    std::optional<std::list<client_t>::iterator> current_client{};
    bool handler_completed = true;

    for(;;)
    {
      {
        bool urgent = current_client.has_value();
        core_lock_t lock(mutex_, urgent);

        if(current_client.has_value())
        {
          // current thread done with handling previous request
          core_.resume_monitoring(*current_client, handler_completed);
          current_client.reset();
          ++n_idle_threads_;
        }

        if(dispatcher_stopping_)
        {
          --n_idle_threads_;
          break;
        }

        // wait for next request or pushed aside
        current_client = core_.select_client();

        if(current_client.has_value())
        {
          // current thread will handle next request from selected client
          if(--n_idle_threads_ == 0)
          {
            // try to start another thread to wait for further requests
            if(current_thread.pool().add(
              [this](pooled_thread_t& added) { this->serve(added); }))
            {
              ++n_idle_threads_;
            }
          }
        }
      }
        
      if(current_client.has_value())
      {
        if(auto msg = context_.message_at(loglevel_t::info))
        {
          *msg << "handling request from connection " <<
            (*current_client)->nb_inbuf() <<
            " on dispatcher thread " << current_thread.id();
        }
        handler_completed = handle_request(current_thread, **current_client);
      }
    }
      
    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "dispatcher thread " << current_thread.id() << " stopped";
    }
  }

private :
  logging_context_t const& context_;
  dispatcher_config_t const config_;
  core_dispatcher_t core_;
  std::size_t n_idle_threads_;
  core_mutex_t mutex_;

  static_assert(std::atomic<bool>::is_always_lock_free);
  std::atomic<bool> dispatcher_stopping_;

  std::unique_ptr<event_pipe_reader_t> signal_reader_;
  std::unique_ptr<event_pipe_writer_t> signal_writer_;
};

dispatcher_t::dispatcher_t(logging_context_t const& context,
                           dispatcher_config_t config)
: impl_(std::make_unique<impl_t>(context, std::move(config)))
{ }

endpoint_t dispatcher_t::add_listener(endpoint_t const& endpoint,
                                      method_map_t const& map)
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
