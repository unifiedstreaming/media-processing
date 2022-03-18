/*
 * Copyright (C) 2022 CodeShop B.V.
 *
 * This file is part of the x264_encoding_service.
 *
 * The x264_encoding_service is free software: you can redistribute it
 * and/or modify it under the terms of version 2 of the GNU General
 * Public License as published by the Free Software Foundation.
 *
 * The x264_encoding_service is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See version 2 of the GNU General Public License for more details.
 *
 * You should have received a copy of version 2 of the GNU General
 * Public License along with the x264_encoding_service.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <cuti/cmdline_reader.hpp>
#include <cuti/dispatcher.hpp>
#include <cuti/endpoint.hpp>
#include <cuti/flag.hpp>
#include <cuti/logger.hpp>
#include <cuti/logging_context.hpp>
#include <cuti/option_walker.hpp>
#include <cuti/resolver.hpp>
#include <cuti/rpc_client.hpp>
#include <cuti/scoped_guard.hpp>
#include <cuti/scoped_thread.hpp>
#include <cuti/streambuf_backend.hpp>

#include <x264es_utils/service.hpp>

#include <csignal>
#include <exception>
#include <iostream>

#undef NDEBUG
#include <cassert>

namespace // anonymous
{

struct x264_client_t
{
  explicit x264_client_t(cuti::endpoint_t const& server_address)
  : rpc_client_(server_address)
  { }

  x264_client_t(x264_client_t const&) = delete;
  x264_client_t& operator=(x264_client_t const&) = delete;

  int add(int arg1, int arg2)
  {
    int result;

    auto inputs = cuti::make_input_list<int>(result);
    auto outputs = cuti::make_output_list<int, int>(arg1, arg2);
    rpc_client_("add", inputs, outputs);

    return result;
  }
  
  int subtract(int arg1, int arg2)
  {
    int result;

    auto inputs = cuti::make_input_list<int>(result);
    auto outputs = cuti::make_output_list<int, int>(arg1, arg2);
    rpc_client_("subtract", inputs, outputs);

    return result;
  }
  
private :
  cuti::rpc_client_t rpc_client_;
};
 

void test_service(cuti::logging_context_t const& client_context,
                  cuti::logging_context_t const& server_context)
{
  if(auto msg = client_context.message_at(cuti::loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  cuti::dispatcher_config_t dispatcher_config;
  auto interfaces = cuti::local_interfaces(cuti::any_port);

  {
    x264es_utils::service_t service(server_context, dispatcher_config,
      interfaces);

    cuti::scoped_thread_t server_thread([&] { service.run(); });
    cuti::scoped_guard_t stop_guard([&] { service.stop(SIGINT); });

    auto const& endpoints = service.endpoints();
    assert(!endpoints.empty());
    x264_client_t client(endpoints.front());

    int result = client.add(42, 4711);
    assert(result == 4753);

    result = client.subtract(4753, 42);
    assert(result == 4711);
  }

  if(auto msg = client_context.message_at(cuti::loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}

struct options_t
{
  static cuti::loglevel_t constexpr default_loglevel =
    cuti::loglevel_t::error;

  options_t()
  : enable_server_logging_(false)
  , loglevel_(default_loglevel)
  { }

  cuti::flag_t enable_server_logging_;
  cuti::loglevel_t loglevel_;
};

void print_usage(std::ostream& os, char const* argv0)
{
  os << "usage: " << argv0 << " [<option> ...]\n";
  os << "options are:\n";
  os << "  --enable-server-logging  enable server-side logging\n";
  os << "  --loglevel <level>       set loglevel " <<
    "(default: " << cuti::loglevel_string(options_t::default_loglevel) <<
    ")\n";
  os << std::flush;
}

void read_options(options_t& options, cuti::option_walker_t& walker)
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
  cuti::cmdline_reader_t reader(argc, argv);
  cuti::option_walker_t walker(reader);

  read_options(options, walker);
  if(!walker.done() || !reader.at_end())
  {
    print_usage(std::cerr, argv[0]);
    return 1;
  }

  cuti::logger_t cerr_logger(
    std::make_unique<cuti::streambuf_backend_t>(std::cerr));
  cuti::logger_t null_logger(nullptr);
  cuti::logging_context_t client_context(cerr_logger, options.loglevel_);
  cuti::logging_context_t server_context(
    options.enable_server_logging_ ? cerr_logger : null_logger,
    options.loglevel_);

  test_service(client_context, server_context);

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
