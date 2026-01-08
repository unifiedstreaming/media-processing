/*
 * Copyright (C) 2024-2026 CodeShop B.V.
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

#include <cuti/simple_nb_client_cache.hpp>

#include <cuti/cmdline_reader.hpp>
#include <cuti/default_scheduler.hpp>
#include <cuti/endpoint.hpp>
#include <cuti/option_walker.hpp>
#include <cuti/resolver.hpp>
#include <cuti/streambuf_backend.hpp>
#include <cuti/scoped_thread.hpp>
#include <cuti/socket_layer.hpp>
#include <cuti/tcp_acceptor.hpp>
#include <cuti/tcp_connection.hpp>

#include <iostream>
#include <sstream>
#include <tuple>
#include <utility>

#undef NDEBUG
#include <cassert>

namespace // anonymous
{

using namespace cuti;

/*
 * A dummy server that just picks up the phone without listening for
 * data or hanging up.
 */
struct dummy_server_t
{
  dummy_server_t(socket_layer_t& sockets)
  : acceptor_(sockets, local_interfaces(sockets, any_port).front())
  , connections_()
  , stop_reader_(nullptr)
  , stop_writer_(nullptr)
  , scheduler_(sockets)
  , stopping_(false)
  , serving_thread_(nullptr)
  {
    acceptor_.set_nonblocking();
    acceptor_.call_when_ready(scheduler_,
      [this](stack_marker_t&) { this->on_acceptor(); });

    std::tie(stop_reader_, stop_writer_) = make_connected_pair(sockets);
    stop_reader_->set_nonblocking();
    stop_reader_->call_when_readable(scheduler_,
      [this](stack_marker_t&) { this->on_stop_reader(); });

    serving_thread_ = std::make_unique<scoped_thread_t>(
      [this] { this->serve(); });
  }

  dummy_server_t(dummy_server_t const&) = delete;
  dummy_server_t& operator=(dummy_server_t const&) = delete;

  endpoint_t const& address() const
  { return acceptor_.local_endpoint(); }
    
  ~dummy_server_t()
  {
    // tell serving thread to stop
    stop_writer_.reset();
  }

private :
  void on_acceptor()
  {
    std::unique_ptr<tcp_connection_t> accepted;
    acceptor_.accept(accepted);

    if(accepted != nullptr)
    {
      connections_.push_back(std::move(accepted));
    }

    acceptor_.call_when_ready(scheduler_,
      [this](stack_marker_t&) { this->on_acceptor(); });
  }
    
  void on_stop_reader()
  {
    char buf[1];
    char *next;
    stop_reader_->read(buf, buf + 1, next);

    if(next == nullptr)
    {
      stop_reader_->call_when_readable(scheduler_,
        [this](stack_marker_t&) { this->on_stop_reader(); });
      return;
    }

    stopping_ = true;
  }
    
  void serve()
  {
    stack_marker_t base_marker;

    while(!stopping_)
    {
      auto cb = scheduler_.wait();
      assert(cb != nullptr);
      cb(base_marker);
    }
  }

private :
  tcp_acceptor_t acceptor_;
  std::vector<std::unique_ptr<tcp_connection_t>> connections_;
  std::unique_ptr<tcp_connection_t> stop_reader_;
  std::unique_ptr<tcp_connection_t> stop_writer_;
  default_scheduler_t scheduler_;
  bool stopping_;
  std::unique_ptr<scoped_thread_t> serving_thread_;
};

std::string connection_id(nb_client_t const& client)
{
  std::stringstream ss;
  ss << client;
  return ss.str();
}
  
void test_dummy_server(logging_context_t const& context)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  {
    socket_layer_t sockets;
    std::vector<std::unique_ptr<nb_client_t>> clients;
    dummy_server_t server(sockets);

    for(int i = 0; i != 100; ++i)
    {
      clients.push_back(
        std::make_unique<nb_client_t>(sockets, server.address()));
    }

    if(auto msg = context.message_at(loglevel_t::info))
    {
      *msg << clients.size() << " clients connected";
    }
  }

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}

void test_single_server_reuse(logging_context_t const& context)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  {
    socket_layer_t sockets;
    simple_nb_client_cache_t cache(sockets);
    dummy_server_t server(sockets);

    auto client_1 = cache.obtain(context, server.address());
    assert(client_1 != nullptr);
    auto id_1 = connection_id(*client_1);

    cache.store(context, std::move(client_1));

    auto client_2 = cache.obtain(context, server.address());
    assert(client_2 != nullptr);
    auto id_2 = connection_id(*client_2);

    assert(id_1 == id_2);
  }
    
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}

void test_multi_server_reuse(logging_context_t const& context)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  {
    socket_layer_t sockets;
    simple_nb_client_cache_t cache(sockets);
    dummy_server_t server_1(sockets);
    dummy_server_t server_2(sockets);

    auto client_1_1 = cache.obtain(context, server_1.address());
    assert(client_1_1 != nullptr);
    auto id_1_1 = connection_id(*client_1_1);

    auto client_2_1 = cache.obtain(context, server_2.address());
    assert(client_2_1 != nullptr);
    auto id_2_1 = connection_id(*client_2_1);

    assert(id_1_1 != id_2_1);

    cache.store(context, std::move(client_1_1));
    cache.store(context, std::move(client_2_1));

    auto client_1_2 = cache.obtain(context, server_1.address());
    assert(client_1_2 != nullptr);
    auto id_1_2 = connection_id(*client_1_2);
    assert(id_1_2 == id_1_1);

    auto client_2_2 = cache.obtain(context, server_2.address());
    assert(client_2_2 != nullptr);
    auto id_2_2 = connection_id(*client_2_2);
    assert(id_2_2 == id_2_1);

    assert(id_1_2 != id_2_2);
  }
    
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}

void test_single_server_invalidation(logging_context_t const& context)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  {
    socket_layer_t sockets;
    simple_nb_client_cache_t cache(sockets);
    dummy_server_t server(sockets);

    auto client_1 = cache.obtain(context, server.address());
    assert(client_1 != nullptr);
    auto id_1 = connection_id(*client_1);

    cache.store(context, std::move(client_1));
    cache.invalidate_entries(context, server.address());

    auto client_2 = cache.obtain(context, server.address());
    assert(client_2 != nullptr);
    auto id_2 = connection_id(*client_2);

    assert(id_1 != id_2);
  }
    
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}

void test_multi_server_invalidation(logging_context_t const& context)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  {
    socket_layer_t sockets;
    simple_nb_client_cache_t cache(sockets);
    dummy_server_t server_1(sockets);
    dummy_server_t server_2(sockets);

    auto client_1_1 = cache.obtain(context, server_1.address());
    assert(client_1_1 != nullptr);
    auto id_1_1 = connection_id(*client_1_1);

    auto client_2_1 = cache.obtain(context, server_2.address());
    assert(client_2_1 != nullptr);
    auto id_2_1 = connection_id(*client_2_1);

    assert(id_1_1 != id_2_1);

    cache.store(context, std::move(client_1_1));
    cache.store(context, std::move(client_2_1));
    cache.invalidate_entries(context, server_1.address());

    auto client_1_2 = cache.obtain(context, server_1.address());
    assert(client_1_2 != nullptr);
    auto id_1_2 = connection_id(*client_1_2);
    assert(id_1_2 != id_1_1);

    auto client_2_2 = cache.obtain(context, server_2.address());
    assert(client_2_2 != nullptr);
    auto id_2_2 = connection_id(*client_2_2);
    assert(id_2_2 == id_2_1);

    assert(id_1_2 != id_2_2);
  }
    
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}

void test_eviction(logging_context_t const& context)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  {
    std::size_t constexpr max_cachesize = 0;

    socket_layer_t sockets;
    simple_nb_client_cache_t cache(sockets, max_cachesize);
    dummy_server_t server(sockets);

    auto client_1 = cache.obtain(context, server.address());
    assert(client_1 != nullptr);
    auto id_1 = connection_id(*client_1);

    cache.store(context, std::move(client_1));

    auto client_2 = cache.obtain(context, server.address());
    assert(client_2 != nullptr);
    auto id_2 = connection_id(*client_2);

    assert(id_1 != id_2);
  }
    
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}

struct options_t
{
  static loglevel_t constexpr default_loglevel = loglevel_t::error;

  options_t()
  : loglevel_(default_loglevel)
  { }

  loglevel_t loglevel_;
};

void print_usage(std::ostream& os, char const* argv0)
{
  os << "usage: " << argv0 << " [<option> ...]\n";
  os << "options are:\n";
  os << "  --loglevel <level>       set loglevel " <<
    "(default: " << loglevel_string(options_t::default_loglevel) << ")\n";
  os << std::flush;
}

void read_options(options_t& options, option_walker_t& walker)
{
  while(!walker.done())
  {
    if(!walker.match("--loglevel", options.loglevel_))
    {
      break;
    }
  }
}

int run_tests(int argc, char const* const* argv)
{
  options_t options;
  cmdline_reader_t reader(argc, argv);
  option_walker_t walker(reader);

  read_options(options, walker);
  if(!walker.done() || !reader.at_end())
  {
    print_usage(std::cerr, argv[0]);
    return 1;
  }

  logger_t logger(std::make_unique<streambuf_backend_t>(std::cerr));
  logging_context_t context(logger, options.loglevel_);

  test_dummy_server(context); // just to be sure

  test_single_server_reuse(context);
  test_multi_server_reuse(context);
  test_single_server_invalidation(context);
  test_multi_server_invalidation(context);
  test_eviction(context);

  return 0;
}
  
} // anonymous

int main(int argc, char* argv[])
{
  try
  {
    return run_tests(argc, argv);
  }
  catch(std::exception const& ex)
  {
    std::cerr << argv[0] << ": exception: " << ex.what() << std::endl;
  }

  return 1;
}
