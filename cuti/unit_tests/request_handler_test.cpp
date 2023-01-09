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

#include <cuti/add_handler.hpp>
#include <cuti/cmdline_reader.hpp>
#include <cuti/default_scheduler.hpp>
#include <cuti/final_result.hpp>
#include <cuti/logging_context.hpp>
#include <cuti/nb_string_inbuf.hpp>
#include <cuti/nb_string_outbuf.hpp>
#include <cuti/option_walker.hpp>
#include <cuti/quoted.hpp>
#include <cuti/request_handler.hpp>
#include <cuti/subtract_handler.hpp>
#include <cuti/streambuf_backend.hpp>

#include <iostream>
#include <limits>
#include <string>

#undef NDEBUG
#include <cassert>

namespace // anonymous
{

using namespace cuti;

struct int_reply_reader_t
{
  using result_value_t = int;

  int_reply_reader_t(result_t<int>& result, bound_inbuf_t& buf)
  : result_(result)
  , value_reader_(*this, result_, buf)
  , eom_checker_(*this, result_, buf)
  , value_()
  { }

  void start()
  {
    value_reader_.start(&int_reply_reader_t::on_value);
  }

private :
  void on_value(int value)
  {
    value_ = value;
    eom_checker_.start(&int_reply_reader_t::on_eom_checked);
  }

  void on_eom_checked()
  {
    result_.submit(value_);
  }

private :
  result_t<int>& result_;
  subroutine_t<int_reply_reader_t, reader_t<int>> value_reader_;
  subroutine_t<int_reply_reader_t, eom_checker_t> eom_checker_;
  int value_;
};

int run_int_request(logging_context_t const& client_context,
                    logging_context_t const& server_context,
                    std::size_t bufsize,
                    method_map_t const& method_map,
                    std::string request)
{
  if(auto msg = client_context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting; request: " << quoted_string(request) <<
      " (bufsize: " << bufsize << ")";
  }

  auto request_inbuf = make_nb_string_inbuf(std::move(request), bufsize);

  std::string reply;
  auto reply_outbuf = make_nb_string_outbuf(reply, bufsize);

  default_scheduler_t scheduler;

  {
    stack_marker_t base_marker;
    bound_inbuf_t bit(base_marker, *request_inbuf, scheduler);
    bound_outbuf_t bot(base_marker, *reply_outbuf, scheduler);

    final_result_t<void> result;
    request_handler_t request_handler(result, server_context,
      bit, bot, method_map);
    request_handler.start();

    while(!result.available())
    {
      auto callback = scheduler.wait();
      assert(callback != nullptr);
      callback();
    }

    result.value();
  }

  if(auto msg = client_context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": reply: " << quoted_string(reply);
  }

  auto reply_inbuf = make_nb_string_inbuf(std::move(reply), bufsize);
  {
    stack_marker_t base_marker;
    bound_inbuf_t bit(base_marker, *reply_inbuf, scheduler);

    final_result_t<int> result;
    int_reply_reader_t reply_reader(result, bit);
    reply_reader.start();

    while(!result.available())
    {
      auto callback = scheduler.wait();
      assert(callback != nullptr);
      callback();
    }

    int value = result.value();
    if(auto msg = client_context.message_at(loglevel_t::info))
    {
      *msg << __func__ << ": done; returning " << value;
    }

    return value;
  }
}

void fail_int_request(logging_context_t const& client_context,
                      logging_context_t const& server_context,
                      std::size_t bufsize,
                      method_map_t const& method_map,
                      std::string request)
{
  bool caught = false;
  try
  {
    run_int_request(client_context, server_context,
      bufsize, method_map, std::move(request));
  }
  catch(std::exception& ex)
  {
    if(auto msg = client_context.message_at(loglevel_t::info))
    {
      *msg << __func__ << ": caught expected exception: " << ex.what();
    }
    caught = true;
  }
  assert(caught);
}
  
void do_run_tests(logging_context_t const& client_context,
                  logging_context_t const& server_context,
                  std::size_t bufsize)
{
  method_map_t map;
  map.add_method_factory(
    "add", default_method_factory<add_handler_t>());
  map.add_method_factory(
    "sub", default_method_factory<subtract_handler_t>());

  assert(run_int_request(client_context, server_context, bufsize, map,
    "add 42 4711 \n") == 4753);
  assert(run_int_request(client_context, server_context, bufsize, map,
    "sub 4753 42 \n") == 4711);

  // no method
  fail_int_request(client_context, server_context, bufsize, map,
    "42 4711 \n");

  // possibly truncated method
  fail_int_request(client_context, server_context, bufsize, map,
    "add");
  fail_int_request(client_context, server_context, bufsize, map,
    "add\n");

  // unknown method
  fail_int_request(client_context, server_context, bufsize, map,
    "mul 42 4711 \n");

  // bad argument type
  fail_int_request(client_context, server_context, bufsize, map,
    "add \"hello\" 4711 \n");

  // missing second argument
  fail_int_request(client_context, server_context, bufsize, map,
    "add 42 \n");

  // int overflow (method failure)
  static auto constexpr max = std::numeric_limits<int>::max(); 
  fail_int_request(client_context, server_context, bufsize, map,
    "add 1 " + std::to_string(max) + " \n");

  // possibly truncated second argument
  fail_int_request(client_context, server_context, bufsize, map,
    "add 42 4711");
  fail_int_request(client_context, server_context, bufsize, map,
    "add 42 4711\n");

  // missing end-of-message
  fail_int_request(client_context, server_context, bufsize, map,
    "add 42 4711 ");
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
