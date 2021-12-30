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

#include <cuti/add_handler.hpp>
#include <cuti/async_readers.hpp>
#include <cuti/cmdline_reader.hpp>
#include <cuti/default_scheduler.hpp>
#include <cuti/final_result.hpp>
#include <cuti/nb_tcp_buffers.hpp>
#include <cuti/option_walker.hpp>
#include <cuti/request_handler.hpp>
#include <cuti/rpc_engine.hpp>
#include <cuti/scoped_thread.hpp>
#include <cuti/stack_marker.hpp>
#include <cuti/streambuf_backend.hpp>
#include <cuti/subtract_handler.hpp>
#include <cuti/tcp_connection.hpp>

#include <iostream>
#include <limits>
#include <memory>
#include <tuple>

#undef NDEBUG
#include <cassert>

namespace // anonymous
{

using namespace cuti;

void run_scheduler(logging_context_t& context, default_scheduler_t& scheduler)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  unsigned int n_callbacks = 0;
  
  callback_t cb;
  while((cb = scheduler.wait()) != nullptr)
  {
    cb();
    ++n_callbacks;
  }

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done; n_callbacks: " << n_callbacks;
  }
}
    
bool at_eof(logging_context_t& context, nb_inbuf_t& inbuf)
{
  default_scheduler_t scheduler;
  stack_marker_t base_marker;
  bound_inbuf_t bound_inbuf(base_marker, inbuf, scheduler);

  final_result_t<bool> result;
  eof_checker_t checker(result, bound_inbuf);
  checker.start();

  run_scheduler(context, scheduler);

  assert(result.available());
  return result.value();
}

void handle_request(logging_context_t& context,
                    nb_inbuf_t& inbuf,
                    nb_outbuf_t& outbuf,
                    method_map_t<request_handler_t> const& method_map)
{
  default_scheduler_t scheduler;
  stack_marker_t base_marker;
  bound_inbuf_t bound_inbuf(base_marker, inbuf, scheduler);
  bound_outbuf_t bound_outbuf(base_marker, outbuf, scheduler);

  final_result_t<void> result;
  request_handler_t handler(
    result, context, bound_inbuf, bound_outbuf, method_map);
  handler.start();

  run_scheduler(context, scheduler);

  assert(result.available());
  result.value();
}
  
void handle_requests(logging_context_t& context,
                     nb_inbuf_t& inbuf,
                     nb_outbuf_t& outbuf,
                     method_map_t<request_handler_t> const& method_map)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  while(!at_eof(context, inbuf))
  {
    handle_request(context, inbuf, outbuf, method_map);
  }
    
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}

void test_add(logging_context_t& context,
              nb_inbuf_t& inbuf,
              nb_outbuf_t& outbuf)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  int result{};
  auto reply_args = make_input_list<int>(result);

  auto request_args = make_output_list<int, int>(42, 4711);

  perform_rpc(inbuf, outbuf, "add", reply_args, request_args);

  assert(result == 4753);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}
  
void test_overflow(logging_context_t& context,
                   nb_inbuf_t& inbuf,
                   nb_outbuf_t& outbuf)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  int result{};
  auto reply_args = make_input_list<int>(result);

  auto request_args = make_output_list<int, int>(
    std::numeric_limits<int>::max(), 1);

  bool caught = false;
  try
  {
    perform_rpc(inbuf, outbuf, "add", reply_args, request_args);
  }
  catch(std::exception const& ex)
  {
    if(auto msg = context.message_at(loglevel_t::info))
    {
      *msg << __func__ << ": caught expected exception: " << ex.what();
    }

    caught = true;
  }
  assert(caught);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}
  
void test_bad_method(logging_context_t& context,
                     nb_inbuf_t& inbuf,
                     nb_outbuf_t& outbuf)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  int result{};
  auto reply_args = make_input_list<int>(result);

  auto request_args = make_output_list<int, int>(42, 4711);

  bool caught = false;
  try
  {
    perform_rpc(inbuf, outbuf, "huh", reply_args, request_args);
  }
  catch(std::exception const& ex)
  {
    if(auto msg = context.message_at(loglevel_t::info))
    {
      *msg << __func__ << ": caught expected exception: " << ex.what();
    }

    caught = true;
  }
  assert(caught);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}
  
void test_sub(logging_context_t& context,
              nb_inbuf_t& inbuf,
              nb_outbuf_t& outbuf)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  int result{};
  auto reply_args = make_input_list<int>(result);

  auto request_args = make_output_list<int, int>(4753, 4711);

  perform_rpc(inbuf, outbuf, "sub", reply_args, request_args);

  assert(result == 42);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}
  
void test_underflow(logging_context_t& context,
                    nb_inbuf_t& inbuf,
                    nb_outbuf_t& outbuf)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  int result{};
  auto reply_args = make_input_list<int>(result);

  auto request_args = make_output_list<int, int>(
    std::numeric_limits<int>::min(), 1);

  bool caught = false;
  try
  {
    perform_rpc(inbuf, outbuf, "sub", reply_args, request_args);
  }
  catch(std::exception const& ex)
  {
    if(auto msg = context.message_at(loglevel_t::info))
    {
      *msg << __func__ << ": caught expected exception: " << ex.what();
    }

    caught = true;
  }
  assert(caught);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}
  
void run_engine_tests(logging_context_t& context,
                      nb_inbuf_t& inbuf,
                      nb_outbuf_t& outbuf)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  test_add(context, inbuf, outbuf);
  test_overflow(context, inbuf, outbuf);
  test_bad_method(context, inbuf, outbuf);
  test_sub(context, inbuf, outbuf);
  test_underflow(context, inbuf, outbuf);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}

void do_run_tests(logging_context_t& client_context,
                  logging_context_t& server_context,
                  std::size_t bufsize)
{
  if(auto msg = client_context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting; bufsize: " << bufsize;
  }

  method_map_t<request_handler_t> map;
  map.add_method_factory(
    "add", default_method_factory<add_handler_t>());
  map.add_method_factory(
    "sub", default_method_factory<subtract_handler_t>());

  std::unique_ptr<tcp_connection_t> server_side;
  std::unique_ptr<tcp_connection_t> client_side;
  std::tie(server_side, client_side) = make_connected_pair();

  {
    /*
     * Destruction order must be:
     *
     * ~client_{in,out} (closing initiates stop of server thread)
     * ~server_thread   (thread no longer references *server_{in,out}) 
     * ~server_{in,out}
     */
    std::unique_ptr<nb_inbuf_t> server_in;
    std::unique_ptr<nb_outbuf_t> server_out;
    std::tie(server_in, server_out) = make_nb_tcp_buffers(
      std::move(server_side), bufsize, bufsize);

    scoped_thread_t server_thread([&]
    { handle_requests(server_context, *server_in, *server_out, map); });
   
    std::unique_ptr<nb_inbuf_t> client_in;
    std::unique_ptr<nb_outbuf_t> client_out;
    std::tie(client_in, client_out) = make_nb_tcp_buffers(
      std::move(client_side), bufsize, bufsize);

    run_engine_tests(client_context, *client_in, *client_out);
  }

  if(auto msg = client_context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
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

  std::size_t constexpr bufsizes[] = { 1, nb_inbuf_t::default_bufsize };
  for(auto bufsize : bufsizes)
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
