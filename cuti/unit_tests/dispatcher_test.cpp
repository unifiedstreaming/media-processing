/*
 * Copyright (C) 2022-2025 CodeShop B.V.
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

#include <cuti/dispatcher.hpp>

#include <cuti/async_readers.hpp>
#include <cuti/chrono_types.hpp>
#include <cuti/cmdline_reader.hpp>
#include <cuti/echo_handler.hpp>
#include <cuti/error_status.hpp>
#include <cuti/logging_context.hpp>
#include <cuti/method_map.hpp>
#include <cuti/option_walker.hpp>
#include <cuti/resolver.hpp>
#include <cuti/rpc_client.hpp>
#include <cuti/scoped_guard.hpp>
#include <cuti/simple_nb_client_cache.hpp>
#include <cuti/stack_marker.hpp>
#include <cuti/streambuf_backend.hpp>
#include <cuti/tcp_connection.hpp>

#include <csignal>
#include <iostream>
#include <list>
#include <thread>
#include <utility>
#include <vector>

#undef NDEBUG
#include <cassert>

namespace // anonymous
{

using namespace cuti;

/*
 * Simple blocking 'sleep' handler
 */
struct sleep_handler_t
{
  using result_value_t = void;

  sleep_handler_t(result_t<void>& result,
                  logging_context_t const& context,
                  bound_inbuf_t& inbuf,
                  bound_outbuf_t& outbuf)
  : result_(result)
  , context_(context)
  , msecs_reader_(*this, result_, inbuf)
  { }

  void start(stack_marker_t& base_marker)
  {
    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "sleep_handler: starting";
    }
    msecs_reader_.start(base_marker, &sleep_handler_t::on_msecs);
  }

private :
  void on_msecs(stack_marker_t& base_marker, unsigned int msecs)
  {
    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "sleep_handler: sleeping for " << msecs << " msecs";
    }

    auto now = cuti_clock_t::now();
    auto limit = now + milliseconds_t(msecs);
    while(now < limit)
    {
      std::this_thread::sleep_for(limit - now);
      now = cuti_clock_t::now();
    }

    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "sleep_handler: done";
    }

    result_.submit(base_marker);
  }
    
private :
  result_t<void>& result_;
  logging_context_t const& context_;
  subroutine_t<sleep_handler_t, reader_t<unsigned int>> msecs_reader_;
};

int send_request(tcp_connection_t& conn, std::string const& request)
{
  char const* first = request.data();
  char const* last = first + request.size();

  int error = 0;
  while(first != last)
  {
    char const* next;
    error = conn.write(first, last, next);
    if(error != 0)
    {
      break;
    }
    assert(next != nullptr);
    first = next;
  }

  return error;
}

std::size_t drain_connection(tcp_connection_t& conn)
{
  std::size_t count = 0;

  char buf[512];
  char* next;
  do
  {
    assert(conn.read(buf, buf + sizeof buf, next) == 0);
    assert(next != nullptr);
    count += next - buf;
  } while(next != buf);

  return count;
}

std::string some_string()
{
  std::string str;

  while(str.size() < 250)
  {
    str += "This is the story of the hare who lost his spectacles... ";
  }

  return str;
}

std::string some_echo_request()
{
  std::string const str = some_string();
  std::string request = "echo [ ";
  while(request.size() < 10000)
  {
    request += "\"";
    request += str;
    request += "\" ";
  }
  request += "] \n";

  return request;
}

std::vector<std::string> some_strings()
{
  std::string const str = some_string();

  std::vector<std::string> result;
  std::size_t total_length = 0;
  while(total_length < 10000)
  {
    result.push_back(str);
    total_length += str.length();
  }
  
  return result;
}

void echo_strings(rpc_client_t& client,
                  std::vector<std::string>& inputs,
                  std::vector<std::string> const& outputs)
{
  auto input_list = make_input_list_ptr<std::vector<std::string>>(inputs);
  auto output_list = make_output_list_ptr<std::vector<std::string>>(outputs);

  client("echo", std::move(input_list), std::move(output_list));
}

void echo_nothing(rpc_client_t& client)
{
  std::vector<std::string> inputs;
  echo_strings(client, inputs, {});
  assert(inputs.empty());
}

void echo_some_strings(rpc_client_t& client)
{
  std::vector<std::string> inputs;
  std::vector<std::string> const outputs = some_strings();
  echo_strings(client, inputs, outputs);
  assert(inputs == outputs);
}
  
void remote_sleep(rpc_client_t& client, unsigned int msecs)
{
  auto inputs = make_input_list_ptr<>();
  auto outputs = make_output_list_ptr<unsigned int>(msecs);

  client("sleep", std::move(inputs), std::move(outputs));
}

void test_deaf_client(logging_context_t const& client_context,
                      logging_context_t const& server_context,
                      std::size_t bufsize)
{
  if(auto msg = client_context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting (bufsize: " << bufsize << ")";
  }

  method_map_t map;
  map.add_method_factory("echo", default_method_factory<echo_handler_t>());

  dispatcher_config_t config;
  config.bufsize_ = bufsize;
  config.throughput_settings_.min_bytes_per_tick_ = 512;
  config.throughput_settings_.low_ticks_limit_ = 10;
  config.throughput_settings_.tick_length_ = milliseconds_t(100);

  {
    dispatcher_t dispatcher(server_context, config);
    endpoint_t server_address = dispatcher.add_listener(
      local_interfaces(any_port).front(), map);

    std::jthread server_thread([&] { dispatcher.run(); });
    auto stop_guard = make_scoped_guard([&] { dispatcher.stop(SIGINT); });
  
    tcp_connection_t client_side(server_address);
    client_side.set_blocking();
  
    std::string const request = some_echo_request();

    if(auto msg = client_context.message_at(loglevel_t::info))
    {
      *msg << __func__ << '(' << client_side << "): flooding server...";
    }

    unsigned int n_requests = 0;
    int error = 0;
    while((error = send_request(client_side, request)) == 0)
    {
      ++n_requests;
    }

    if(auto msg = client_context.message_at(loglevel_t::info))
    {
      *msg << __func__ << '(' << client_side <<
        "): got expected error after sending " << n_requests <<
        " requests: " << error_status_t(error);
    }
  }

  if(auto msg = client_context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}

void test_slow_client(logging_context_t const& client_context,
                      logging_context_t const& server_context,
                      std::size_t bufsize)
{
  if(auto msg = client_context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting (bufsize: " << bufsize << ")";
  }

  method_map_t map;
  map.add_method_factory("echo", default_method_factory<echo_handler_t>());

  dispatcher_config_t config;
  config.bufsize_ = bufsize;
  config.throughput_settings_.min_bytes_per_tick_ = 512;
  config.throughput_settings_.low_ticks_limit_ = 10;
  config.throughput_settings_.tick_length_ = milliseconds_t(10);

  {
    dispatcher_t dispatcher(server_context, config);
    endpoint_t server_address = dispatcher.add_listener(
      local_interfaces(any_port).front(), map);

    std::jthread server_thread([&] { dispatcher.run(); });
    auto stop_guard = make_scoped_guard([&] { dispatcher.stop(SIGINT); });
  
    tcp_connection_t client_side(server_address);
    client_side.set_blocking();
  
    std::string const incomplete_request = "echo [ \"hello";

    if(auto msg = client_context.message_at(loglevel_t::info))
    {
      *msg << __func__ << '(' << client_side <<
        "): sending incomplete request...";
    }
    assert(send_request(client_side, incomplete_request) == 0);

    // wait for eof caused by server-side request reading timeout
    auto bytes_received = drain_connection(client_side);
    if(auto msg = client_context.message_at(loglevel_t::info))
    {
      *msg << __func__ << '(' << client_side <<
      "): got expected eof after receiving " << bytes_received <<
      " bytes";
    }
  }

  if(auto msg = client_context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}

void test_eviction(logging_context_t const& client_context,
                   logging_context_t const& server_context,
                   std::size_t bufsize)
{
  if(auto msg = client_context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting (bufsize: " << bufsize << ")";
  }

  method_map_t map;
  map.add_method_factory("echo", default_method_factory<echo_handler_t>());

  dispatcher_config_t config;
  config.bufsize_ = bufsize;
  config.max_concurrent_requests_ = 1;
  config.max_connections_ = 1;

  {
    dispatcher_t dispatcher(server_context, config);
    endpoint_t server_address = dispatcher.add_listener(
      local_interfaces(any_port).front(), map);

    std::jthread server_thread([&] { dispatcher.run(); });
    auto stop_guard = make_scoped_guard([&] { dispatcher.stop(SIGINT); });

    simple_nb_client_cache_t cache1(
      simple_nb_client_cache_t::default_max_cachesize, bufsize, bufsize);
    rpc_client_t client1(client_context, cache1, server_address);
    echo_nothing(client1);

    simple_nb_client_cache_t cache2(
      simple_nb_client_cache_t::default_max_cachesize, bufsize, bufsize);
    rpc_client_t client2(client_context, cache2, server_address);
    echo_nothing(client2);

    bool caught = false;
    try
    {
      echo_nothing(client1);
    }
    catch(const std::exception& ex)
    {
      if(auto msg = client_context.message_at(loglevel_t::info))
      {
        *msg << __func__ << ": got expected exception: " << ex.what();
      }
      caught = true;
    }
    assert(caught);
  }
  
  if(auto msg = client_context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}

void test_remote_sleeps(logging_context_t const& client_context,
                        logging_context_t const& server_context,
                        std::size_t bufsize,
                        std::size_t max_concurrent_requests,
                        std::size_t n_clients)
{
  if(auto msg = client_context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting (bufsize: " << bufsize <<
      " max_concurrent_requests: " << max_concurrent_requests <<
      " n_clients: " << n_clients << ")";
  }

  method_map_t map;
  map.add_method_factory("sleep", default_method_factory<sleep_handler_t>());

  dispatcher_config_t config;
  config.bufsize_ = bufsize;
  config.max_concurrent_requests_ = max_concurrent_requests;

  {
    dispatcher_t dispatcher(server_context, config);
    endpoint_t server_address = dispatcher.add_listener(
      local_interfaces(any_port).front(), map);

    std::jthread server_thread([&] { dispatcher.run(); });
    auto stop_guard = make_scoped_guard([&] { dispatcher.stop(SIGINT); });

    simple_nb_client_cache_t cache(
      simple_nb_client_cache_t::default_max_cachesize, bufsize, bufsize);

    std::list<rpc_client_t> clients;
    while(clients.size() != n_clients)
    {
      clients.emplace_back(client_context, cache, server_address);
    }

    std::list<std::jthread> client_threads;
    for(auto& client : clients)
    {
      client_threads.emplace_back([&]
      {
        for(int i = 0; i != 4; ++i)
        {
          remote_sleep(client, 25);
        }
      });
    }
  }
  
  if(auto msg = client_context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}

void test_concurrent_requests(logging_context_t const& client_context,
                              logging_context_t const& server_context,
                              std::size_t bufsize)
{
  if(auto msg = client_context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting (bufsize: " << bufsize << ")";
  }

  test_remote_sleeps(client_context, server_context, bufsize,
    dispatcher_config_t::default_max_concurrent_requests(),
    dispatcher_config_t::default_max_concurrent_requests());

  if(auto msg = client_context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}

void test_full_thread_pool(logging_context_t const& client_context,
                           logging_context_t const& server_context,
                           std::size_t bufsize)
{
  if(auto msg = client_context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting (bufsize: " << bufsize << ")";
  }

  test_remote_sleeps(client_context, server_context, bufsize,
    dispatcher_config_t::default_max_concurrent_requests() / 2,
    dispatcher_config_t::default_max_concurrent_requests());

  if(auto msg = client_context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}

void do_test_interrupted_server(logging_context_t const& client_context,
                                logging_context_t const& server_context,
                                std::size_t bufsize,
                                std::size_t max_concurrent_requests,
                                std::size_t n_clients)
{
  if(auto msg = client_context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting (bufsize: " << bufsize <<
      " max_concurrent_requests: " << max_concurrent_requests <<
      " n_clients: " << n_clients << ")";
  }

  method_map_t map;
  map.add_method_factory("echo", default_method_factory<echo_handler_t>());

  dispatcher_config_t config;
  config.bufsize_ = bufsize;
  config.max_concurrent_requests_ = max_concurrent_requests;

  {
    std::optional<dispatcher_t> dispatcher;
    dispatcher.emplace(server_context, config);
    endpoint_t server_address = dispatcher->add_listener(
      local_interfaces(any_port).front(), map);

    simple_nb_client_cache_t cache(
      simple_nb_client_cache_t::default_max_cachesize, bufsize, bufsize);

    std::list<std::jthread> client_threads;
    for(std::size_t i = 0; i != n_clients; ++i)
    {
      client_threads.emplace_back([&]
      {
        unsigned int n_calls = 0;
        try
        {
          rpc_client_t client(client_context, cache, server_address);
          for(;;)
          {
            echo_some_strings(client);
            ++n_calls;
          }
        }
        catch(std::exception const& ex)
        {
          if(auto msg = client_context.message_at(loglevel_t::info))
          {
            *msg << __func__ << ": caught expected exception after " <<
              n_calls << " calls: " << ex.what();
          }
        }
      });
    }  

    {
      std::jthread stopper([&dispatcher]
      {
        std::this_thread::sleep_for(milliseconds_t(1000));
        dispatcher->stop(SIGINT);
      });

      dispatcher->run();
    }

    dispatcher.reset();
  }
  
  if(auto msg = client_context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}

void test_interrupted_server(logging_context_t const& client_context,
                             logging_context_t const& server_context,
                             std::size_t bufsize)
{
  if(auto msg = client_context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting (bufsize: " << bufsize << ")";
  }

  do_test_interrupted_server(client_context, server_context, bufsize,
    dispatcher_config_t::default_max_concurrent_requests(),
    dispatcher_config_t::default_max_concurrent_requests());

  if(auto msg = client_context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}

void test_overloaded_interrupted_server(
  logging_context_t const& client_context,
  logging_context_t const& server_context,
  std::size_t bufsize)
{
  if(auto msg = client_context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting (bufsize: " << bufsize << ")";
  }

  do_test_interrupted_server(client_context, server_context, bufsize,
    dispatcher_config_t::default_max_concurrent_requests() / 2,
    dispatcher_config_t::default_max_concurrent_requests());

  if(auto msg = client_context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}

void test_restart(logging_context_t const& client_context,
                  logging_context_t const& server_context,
                  std::size_t bufsize)
{
  method_map_t map;
  map.add_method_factory("echo", default_method_factory<echo_handler_t>());

  dispatcher_config_t config;
  config.bufsize_ = bufsize;

  if(auto msg = client_context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting (bufsize: " << bufsize << ")";
  }

  {
    dispatcher_t dispatcher(server_context, config);
    endpoint_t server_address = dispatcher.add_listener(
      local_interfaces(any_port).front(), map);

    /*
     * In theory, restarting a stopped dispatcher should be possible.
     * However, the connections associated with any currently running
     * requests may be lost.  Here, we do not have such connnections.
     */
    for(int i = 0; i != 2; ++i)
    {
      simple_nb_client_cache_t cache(
        simple_nb_client_cache_t::default_max_cachesize, bufsize, bufsize);

      std::jthread runner(
        [&dispatcher] { dispatcher.run(); });
      auto stop_guard = make_scoped_guard(
        [&dispatcher] { dispatcher.stop(SIGINT); });

      rpc_client_t client(client_context, cache, server_address);
      echo_nothing(client);
    }
  }
    
  if(auto msg = client_context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}

void do_run_tests(logging_context_t const& client_context,
                  logging_context_t const& server_context,
                  std::size_t bufsize)
{
  test_deaf_client(client_context, server_context, bufsize);
  test_slow_client(client_context, server_context, bufsize);
  test_eviction(client_context, server_context, bufsize);
  test_concurrent_requests(client_context, server_context, bufsize);
  test_full_thread_pool(client_context, server_context, bufsize);
  test_interrupted_server(client_context, server_context, bufsize);
  test_overloaded_interrupted_server(client_context, server_context, bufsize);
  test_restart(client_context, server_context, bufsize);
}

struct options_t
{
  static loglevel_t constexpr default_loglevel = loglevel_t::error;

  options_t()
  : enable_server_logging_(false)
  , loglevel_(default_loglevel)
  { }

  flag_t enable_server_logging_;
  loglevel_t loglevel_;
};

void print_usage(std::ostream& os, char const* argv0)
{
  os << "usage: " << argv0 << " [<option> ...]\n";
  os << "options are:\n";
  os << "  --enable-server-logging  enable server-side logging\n";
  os << "  --loglevel <level>       set loglevel " <<
    "(default: " << loglevel_string(options_t::default_loglevel) << ")\n";
  os << std::flush;
}

void read_options(options_t& options, option_walker_t& walker)
{
  while(!walker.done())
  {
    if(!walker.match(
         "--enable-server-logging", options.enable_server_logging_) &&
       !walker.match(
         "--loglevel", options.loglevel_))
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

  logger_t cerr_logger(std::make_unique<streambuf_backend_t>(std::cerr));
  logger_t null_logger(nullptr);
  logging_context_t client_context(cerr_logger, options.loglevel_);
  logging_context_t server_context(
    options.enable_server_logging_ ? cerr_logger : null_logger,
    options.loglevel_);

  static std::size_t constexpr bufsizes[] =
    { 512, dispatcher_config_t::default_bufsize() };

  for(auto bufsize: bufsizes)
  {
    do_run_tests(client_context, server_context, bufsize);
  }
  
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
