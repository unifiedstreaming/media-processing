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

#undef NDEBUG
#include <cassert>

namespace // anonymous
{

using namespace cuti;

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
  std::string str = some_string();
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

void do_run_tests(logging_context_t const& client_context,
                  logging_context_t const& server_context,
                  std::size_t bufsize)
{
  test_deaf_client(client_context, server_context, bufsize);
  test_slow_client(client_context, server_context, bufsize);
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
