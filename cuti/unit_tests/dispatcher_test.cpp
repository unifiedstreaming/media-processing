/*
 * Copyright (C) 2022 CodeShop B.V.
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
#include <cuti/logging_context.hpp>
#include <cuti/method_map.hpp>
#include <cuti/nb_tcp_buffers.hpp>
#include <cuti/option_walker.hpp>
#include <cuti/resolver.hpp>
#include <cuti/rpc_engine.hpp>
#include <cuti/scoped_guard.hpp>
#include <cuti/scoped_thread.hpp>
#include <cuti/streambuf_backend.hpp>
#include <cuti/system_error.hpp>
#include <cuti/tcp_connection.hpp>

#include <csignal>
#include <iostream>
#include <list>
#include <thread>
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

  void start()
  {
    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "sleep_handler: starting";
    }
    msecs_reader_.start(&sleep_handler_t::on_msecs);
  }

private :
  void on_msecs(unsigned int msecs)
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

    result_.submit();
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
  request += "]\n";

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

void echo_strings(nb_inbuf_t& inbuf,
                  std::vector<std::string>& inputs,
                  nb_outbuf_t& outbuf,
                  std::vector<std::string> const& outputs)
{
  auto input_list = make_input_list<std::vector<std::string>>(inputs);
  auto output_list = make_output_list<std::vector<std::string>>(outputs);

  perform_rpc("echo", inbuf, input_list, outbuf, output_list);
}

void echo_nothing(nb_inbuf_t& inbuf, nb_outbuf_t& outbuf)
{
  std::vector<std::string> inputs;
  echo_strings(inbuf, inputs, outbuf, {});
  assert(inputs.empty());
}

void echo_some_strings(nb_inbuf_t& inbuf, nb_outbuf_t& outbuf)
{
  std::vector<std::string> inputs;
  std::vector<std::string> const outputs = some_strings();
  echo_strings(inbuf, inputs, outbuf, outputs);
  assert(inputs == outputs);
}
  
void remote_sleep(nb_inbuf_t& inbuf, nb_outbuf_t& outbuf, unsigned int msecs)
{
  auto input_args = make_input_list<>();
  auto output_args = make_output_list<unsigned int>(msecs);

  perform_rpc("sleep", inbuf, input_args, outbuf, output_args);
}

void test_deaf_client(logging_context_t const& client_context,
                      logging_context_t const& server_context,
                      std::size_t bufsize)
{
  method_map_t map;
  map.add_method_factory("echo", default_method_factory<echo_handler_t>());

  dispatcher_config_t config;
  config.bufsize_ = bufsize;
  config.throughput_settings_.min_bytes_per_tick_ = 512;
  config.throughput_settings_.low_ticks_limit_ = 10;
  config.throughput_settings_.tick_length_ = milliseconds_t(100);

  dispatcher_t dispatcher(server_context, config);

  endpoint_t server_address = dispatcher.add_listener(
    local_interfaces(any_port).front(), map);

  tcp_connection_t client_side(server_address);
  client_side.set_blocking();
  
  std::string const request = some_echo_request();

  if(auto msg = client_context.message_at(loglevel_t::info))
  {
    *msg << __func__ << '(' << client_side <<
      "): flooding server (bufsize: " << bufsize << ")...";
  }

  unsigned int n_requests = 0;
  int error = 0;
  {
    scoped_thread_t server_thread([&] { dispatcher.run(); });
    auto stop_guard = make_scoped_guard([&] { dispatcher.stop(SIGINT); });
  
    while((error = send_request(client_side, request)) == 0)
    {
      ++n_requests;
    }
  }

  if(auto msg = client_context.message_at(loglevel_t::info))
  {
    *msg << __func__ << '(' << client_side <<
      "): got expected error after sending " << n_requests <<
      " requests: " << system_error_string(error);
  }
}

void test_slow_client(logging_context_t const& client_context,
                      logging_context_t const& server_context,
                      std::size_t bufsize)
{
  method_map_t map;
  map.add_method_factory("echo", default_method_factory<echo_handler_t>());

  dispatcher_config_t config;
  config.bufsize_ = bufsize;
  config.throughput_settings_.min_bytes_per_tick_ = 512;
  config.throughput_settings_.low_ticks_limit_ = 10;
  config.throughput_settings_.tick_length_ = milliseconds_t(10);

  dispatcher_t dispatcher(server_context, config);

  endpoint_t server_address = dispatcher.add_listener(
    local_interfaces(any_port).front(), map);

  tcp_connection_t client_side(server_address);
  client_side.set_blocking();
  
  std::string const incomplete_request = "echo [ \"hello";

  if(auto msg = client_context.message_at(loglevel_t::info))
  {
    *msg << __func__ << '(' << client_side <<
      "): testing incomplete request (bufsize: " << bufsize << ")...";
  }

  std::size_t bytes_received = 0;
  {
    scoped_thread_t server_thread([&] { dispatcher.run(); });
    auto stop_guard = make_scoped_guard([&] { dispatcher.stop(SIGINT); });
  
    assert(send_request(client_side, incomplete_request) == 0);

    // wait for eof caused by server-side request reading timeout
    bytes_received = drain_connection(client_side);
  }

  if(auto msg = client_context.message_at(loglevel_t::info))
  {
    *msg << __func__ << '(' << client_side <<
      "): got expected eof after receiving " << bytes_received <<
      " bytes";
  }
}

void test_eviction(logging_context_t const& client_context,
                   logging_context_t const& server_context,
                   std::size_t bufsize)
{
  method_map_t map;
  map.add_method_factory("echo", default_method_factory<echo_handler_t>());

  dispatcher_config_t config;
  config.bufsize_ = bufsize;
  config.max_thread_pool_size_ = 1;
  config.max_connections_ = 1;

  dispatcher_t dispatcher(server_context, config);

  endpoint_t server_address = dispatcher.add_listener(
    local_interfaces(any_port).front(), map);

  {
    scoped_thread_t server_thread([&] { dispatcher.run(); });
    auto stop_guard = make_scoped_guard([&] { dispatcher.stop(SIGINT); });

    if(auto msg = client_context.message_at(loglevel_t::info))
    {
      *msg << __func__ << '(' << server_address <<
        "): trying two clients (bufsize: " << bufsize << ")...";
    }

    std::unique_ptr<nb_inbuf_t> inbuf1;
    std::unique_ptr<nb_outbuf_t> outbuf1;
    std::tie(inbuf1, outbuf1) = make_nb_tcp_buffers(
      std::make_unique<tcp_connection_t>(server_address), bufsize, bufsize);

    if(auto msg = client_context.message_at(loglevel_t::info))
    {
      *msg << __func__ << ": using connection " << *inbuf1;
    }
    echo_nothing(*inbuf1, *outbuf1);

    std::unique_ptr<nb_inbuf_t> inbuf2;
    std::unique_ptr<nb_outbuf_t> outbuf2;
    std::tie(inbuf2, outbuf2) = make_nb_tcp_buffers(
      std::make_unique<tcp_connection_t>(server_address), bufsize, bufsize);

    if(auto msg = client_context.message_at(loglevel_t::info))
    {
      *msg << __func__ << ": using connection " << *inbuf2;
    }
    echo_nothing(*inbuf2, *outbuf2);

    bool caught = false;
    try
    {
      if(auto msg = client_context.message_at(loglevel_t::info))
      {
        *msg << __func__ << ": re-using connection " << *inbuf1;
      }
      echo_nothing(*inbuf1, *outbuf1);
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
                        std::size_t max_thread_pool_size,
                        std::size_t n_clients)
{
  if(auto msg = client_context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting (bufsize: " << bufsize <<
      " max_thread_pool_size: " << max_thread_pool_size <<
      " n_clients: " << n_clients << ")";
  }

  method_map_t map;
  map.add_method_factory("sleep", default_method_factory<sleep_handler_t>());

  dispatcher_config_t config;
  config.bufsize_ = bufsize;
  config.max_thread_pool_size_ = max_thread_pool_size;

  dispatcher_t dispatcher(server_context, config);
  endpoint_t server_address = dispatcher.add_listener(
    local_interfaces(any_port).front(), map);

  {
    scoped_thread_t server_thread([&] { dispatcher.run(); });
    auto stop_guard = make_scoped_guard([&] { dispatcher.stop(SIGINT); });

    std::vector<
      std::pair<std::unique_ptr<nb_inbuf_t>, std::unique_ptr<nb_outbuf_t>>
    > clients;
    while(clients.size() != n_clients)
    {
      clients.push_back(make_nb_tcp_buffers(
        std::make_unique<tcp_connection_t>(server_address), bufsize, bufsize));
    }

    std::list<scoped_thread_t> client_threads;
    for(auto& client : clients)
    {
      client_threads.emplace_back([&]
      {
        for(int i = 0; i != 4; ++i)
        {
          remote_sleep(*client.first, *client.second, 25);
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
    dispatcher_config_t::default_max_thread_pool_size(),
    dispatcher_config_t::default_max_thread_pool_size());

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
    dispatcher_config_t::default_max_thread_pool_size() / 2,
    dispatcher_config_t::default_max_thread_pool_size());

  if(auto msg = client_context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}

void do_test_interrupted_server(logging_context_t const& client_context,
                                logging_context_t const& server_context,
                                std::size_t bufsize,
                                std::size_t max_thread_pool_size,
                                std::size_t n_clients)
{
  if(auto msg = client_context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting (bufsize: " << bufsize <<
      " max_thread_pool_size: " << max_thread_pool_size <<
      " n_clients: " << n_clients << ")";
  }

  method_map_t map;
  map.add_method_factory("echo", default_method_factory<echo_handler_t>());

  dispatcher_config_t config;
  config.bufsize_ = bufsize;
  config.max_thread_pool_size_ = max_thread_pool_size;

  auto dispatcher = std::make_unique<dispatcher_t>(server_context, config);
  endpoint_t server_address = dispatcher->add_listener(
    local_interfaces(any_port).front(), map);

  {
    scoped_thread_t dispatcher_runner([dispatcher = std::move(dispatcher)]
    {
      scoped_thread_t interrupter([&dispatcher]
      {
        std::this_thread::sleep_for(milliseconds_t(1000));
        dispatcher->stop(SIGINT);
      });
      dispatcher->run();
    });

    std::vector<
      std::pair<std::unique_ptr<nb_inbuf_t>, std::unique_ptr<nb_outbuf_t>>
    > clients;
    while(clients.size() != n_clients)
    {
      clients.push_back(make_nb_tcp_buffers(
        std::make_unique<tcp_connection_t>(server_address), bufsize, bufsize));
    }

    std::list<scoped_thread_t> client_threads;
    for(auto& client : clients)
    {
      client_threads.emplace_back([&]
      {
        try
        {
          for(;;)
          {
            echo_some_strings(*client.first, *client.second);
          }
        }
        catch(std::exception const& ex)
        {
          if(auto msg = client_context.message_at(loglevel_t::info))
          {
            *msg << __func__ << ": caught expected exception: " << ex.what();
          }
        }
      });
    }
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
    dispatcher_config_t::default_max_thread_pool_size(),
    dispatcher_config_t::default_max_thread_pool_size());

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
    dispatcher_config_t::default_max_thread_pool_size() / 2,
    dispatcher_config_t::default_max_thread_pool_size());

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

    std::unique_ptr<nb_inbuf_t> client_in;
    std::unique_ptr<nb_outbuf_t> client_out;
    std::tie(client_in, client_out) = make_nb_tcp_buffers(
      std::make_unique<tcp_connection_t>(server_address), bufsize, bufsize);

    /*
     * In theory, restarting a stopped dispatcher should be possible.
     * However, the connections associated with any currently running
     * requests will be lost.  Here, we do not have such connnections.
     */
    for(int i = 0; i != 2; ++i)
    {
      scoped_thread_t runner(
        [&dispatcher] { dispatcher.run(); });
      auto stop_guard = make_scoped_guard(
        [&dispatcher] { dispatcher.stop(SIGINT); });
      echo_nothing(*client_in, *client_out);
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
