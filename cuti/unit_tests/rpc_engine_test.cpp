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

#include <cuti/add_handler.hpp>
#include <cuti/async_readers.hpp>
#include <cuti/cmdline_reader.hpp>
#include <cuti/default_scheduler.hpp>
#include <cuti/echo_handler.hpp>
#include <cuti/final_result.hpp>
#include <cuti/nb_tcp_buffers.hpp>
#include <cuti/option_walker.hpp>
#include <cuti/quoted_string.hpp>
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
#include <stdexcept>
#include <tuple>
#include <utility>

#undef NDEBUG
#include <cassert>

namespace // anonymous
{

using namespace cuti;

size_t constexpr n_echo_args = 100; 
char constexpr censored[] = "*** CENSORED ***";

std::vector<std::string> make_echo_args()
{
  std::vector<std::string> result;
  result.reserve(n_echo_args);

  for(unsigned int i = 0; i != n_echo_args; ++i)
  {
    if(i == n_echo_args / 2)
    {
      result.push_back(censored);
    }
    else
    {
      result.push_back(
        "A man, a plan, a canal: Panama! (" + std::to_string(i) + ")");
    }
  }
  
  return result;
}

std::vector<std::string> const echo_args = make_echo_args();

struct string_source_t
{
  explicit string_source_t(
    logging_context_t const& context,
    std::optional<std::size_t> error_index = std::nullopt)
  : context_(context)
  , first_(echo_args.begin())
  , last_(echo_args.end())
  , error_index_(error_index)
  { }

  std::optional<std::string> operator()()
  {
    std::optional<std::string> result;

    if(first_ != last_)
    {
      if(error_index_ != std::nullopt)
      {
        if(*error_index_ == 0)
        {
          if(auto msg = context_.message_at(loglevel_t::info))
          {
            *msg << "string_source: forcing output error";
          }
          throw std::runtime_error("forced output error");
        }

        --(*error_index_);
      }

      if(auto msg = context_.message_at(loglevel_t::info))
      {
        *msg << "string_source: producing " << quoted_string(*first_);
      }
      result.emplace(*first_);
      ++first_;
    }
    else
    {
      if(auto msg = context_.message_at(loglevel_t::info))
      {
        *msg << "string_source: producing nullopt";
      }
    }
    
    return result;
  }

private :
  logging_context_t const& context_;
  std::vector<std::string>::const_iterator first_;
  std::vector<std::string>::const_iterator last_;
  std::optional<std::size_t> error_index_;
};

struct string_sink_t
{
  explicit string_sink_t(logging_context_t const& context,
                         std::vector<std::string>& target,
                         std::optional<std::size_t> error_index = std::nullopt)
  : context_(context)
  , target_(target)
  , error_index_(error_index)
  { }

  void operator()(std::optional<std::string> value)
  {
    if(value != std::nullopt)
    {
      if(error_index_ != std::nullopt)
      {
        if(*error_index_ == 0)
        {
          if(auto msg = context_.message_at(loglevel_t::info))
          {
            *msg << "string_sink: forcing input error";
          }
          throw std::runtime_error("forced input error");
        }

        --(*error_index_);
      }
        
      if(auto msg = context_.message_at(loglevel_t::info))
      {
        *msg << "string_sink: consuming " << quoted_string(*value);
      }
      target_.push_back(std::move(*value));
    }
    else
    {
      if(auto msg = context_.message_at(loglevel_t::info))
      {
        *msg << "string_sink: consuming nullopt";
      }
    }
  }

private :
  logging_context_t const& context_;
  std::vector<std::string>& target_;
  std::optional<std::size_t> error_index_;
};
      
void run_scheduler(logging_context_t const& context,
                   default_scheduler_t& scheduler)
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
    
bool at_eof(logging_context_t const& context, nb_inbuf_t& inbuf)
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

void handle_request(logging_context_t const& context,
                    nb_inbuf_t& inbuf,
                    nb_outbuf_t& outbuf,
                    method_map_t const& method_map)
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
  
void handle_requests(logging_context_t const& context,
                     nb_inbuf_t& inbuf,
                     nb_outbuf_t& outbuf,
                     method_map_t const& method_map)
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

template<typename... ReplyArgs, typename... RequestArgs>
void check_rpc_failure(logging_context_t const& context,
                       nb_inbuf_t& inbuf,
                       nb_outbuf_t& outbuf,
                       identifier_t method,
                       input_list_t<ReplyArgs...>& reply_args,
                       output_list_t<RequestArgs...>& request_args)
{
  bool caught = false;
  try
  {
    perform_rpc(inbuf, outbuf, std::move(method), reply_args, request_args);
  }
  catch(std::exception const& ex)
  {
    caught = true;

    if(auto msg = context.message_at(loglevel_t::info))
    {
      *msg << __func__ << ": caught expected exception: " << ex.what();
    }
  }
  assert(caught);
}

void test_add(logging_context_t const& context,
              nb_inbuf_t& inbuf,
              nb_outbuf_t& outbuf)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  int reply{};
  auto reply_args = make_input_list<int>(reply);

  auto request_args = make_output_list<int, int>(42, 4711);

  perform_rpc(inbuf, outbuf, "add", reply_args, request_args);

  assert(reply == 4753);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}
  
void test_overflow(logging_context_t const& context,
                   nb_inbuf_t& inbuf,
                   nb_outbuf_t& outbuf)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  int reply{};
  auto reply_args = make_input_list<int>(reply);

  auto request_args = make_output_list<int, int>(
    std::numeric_limits<int>::max(), 1);

  check_rpc_failure(context, inbuf, outbuf, "add", reply_args, request_args);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}
  
void test_bad_method(logging_context_t const& context,
                     nb_inbuf_t& inbuf,
                     nb_outbuf_t& outbuf)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  int reply{};
  auto reply_args = make_input_list<int>(reply);

  auto request_args = make_output_list<int, int>(42, 4711);

  check_rpc_failure(context, inbuf, outbuf, "huh", reply_args, request_args);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}
  
void test_subtract(logging_context_t const& context,
                   nb_inbuf_t& inbuf,
                   nb_outbuf_t& outbuf)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  int reply{};
  auto reply_args = make_input_list<int>(reply);

  auto request_args = make_output_list<int, int>(4753, 4711);

  perform_rpc(inbuf, outbuf, "subtract", reply_args, request_args);

  assert(reply == 42);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}
  
void test_underflow(logging_context_t const& context,
                    nb_inbuf_t& inbuf,
                    nb_outbuf_t& outbuf)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  int reply{};
  auto reply_args = make_input_list<int>(reply);

  auto request_args = make_output_list<int, int>(
    std::numeric_limits<int>::min(), 1);

  check_rpc_failure(context, inbuf, outbuf, "subtract",
    reply_args, request_args);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}

void test_vector_echo(logging_context_t const& context,
                      nb_inbuf_t& inbuf,
                      nb_outbuf_t& outbuf)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  std::vector<std::string> reply;
  auto reply_args = make_input_list<std::vector<std::string>>(reply);

  auto request_args = make_output_list<std::vector<std::string>>(echo_args);

  perform_rpc(inbuf, outbuf, "echo", reply_args, request_args);

  assert(reply == echo_args);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}
  
void test_vector_censored_echo(logging_context_t const& context,
                               nb_inbuf_t& inbuf,
                               nb_outbuf_t& outbuf)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  std::vector<std::string> reply;
  auto reply_args = make_input_list<std::vector<std::string>>(reply);

  auto request_args = make_output_list<std::vector<std::string>>(echo_args);

  check_rpc_failure(context, inbuf, outbuf, "censored_echo",
    reply_args, request_args);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}
  
void test_streaming_echo(logging_context_t const& context,
                         nb_inbuf_t& inbuf,
                         nb_outbuf_t& outbuf)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  std::vector<std::string> reply;
  auto reply_args = make_input_list<streaming_tag_t<std::string>>(
    string_sink_t(context, reply));

  auto request_args = make_output_list<streaming_tag_t<std::string>>(
    string_source_t(context));

  perform_rpc(inbuf, outbuf, "echo", reply_args, request_args);

  assert(reply == echo_args);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}
  
void test_streaming_censored_echo(logging_context_t const& context,
                                  nb_inbuf_t& inbuf,
                                  nb_outbuf_t& outbuf)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  std::vector<std::string> reply;
  auto reply_args = make_input_list<streaming_tag_t<std::string>>(
    string_sink_t(context, reply));

  auto request_args = make_output_list<streaming_tag_t<std::string>>(
    string_source_t(context));

  check_rpc_failure(context, inbuf, outbuf, "censored_echo",
    reply_args, request_args);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}
  
void test_streaming_output_error(logging_context_t const& context,
                                 nb_inbuf_t& inbuf,
                                 nb_outbuf_t& outbuf)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  std::vector<std::string> reply;
  auto reply_args = make_input_list<streaming_tag_t<std::string>>(
    string_sink_t(context, reply));

  auto request_args = make_output_list<streaming_tag_t<std::string>>(
    string_source_t(context, n_echo_args / 2));

  check_rpc_failure(context, inbuf, outbuf, "echo",
    reply_args, request_args);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}
  
void test_streaming_input_error(logging_context_t const& context,
                                nb_inbuf_t& inbuf,
                                nb_outbuf_t& outbuf)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  std::vector<std::string> reply;
  auto reply_args = make_input_list<streaming_tag_t<std::string>>(
    string_sink_t(context, reply, n_echo_args / 2));

  auto request_args = make_output_list<streaming_tag_t<std::string>>(
    string_source_t(context));

  check_rpc_failure(context, inbuf, outbuf, "echo",
    reply_args, request_args);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}
  
void test_streaming_multiple_errors(logging_context_t const& context,
                                    nb_inbuf_t& inbuf,
                                    nb_outbuf_t& outbuf)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  std::vector<std::string> reply;
  auto reply_args = make_input_list<streaming_tag_t<std::string>>(
    string_sink_t(context, reply, n_echo_args / 4));

  auto request_args = make_output_list<streaming_tag_t<std::string>>(
    string_source_t(context, 3 * (n_echo_args / 4)));

  check_rpc_failure(context, inbuf, outbuf, "censored_echo",
    reply_args, request_args);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}
  
void run_engine_tests(logging_context_t const& context,
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
  test_subtract(context, inbuf, outbuf);
  test_underflow(context, inbuf, outbuf);
  test_vector_echo(context, inbuf, outbuf);
  test_vector_censored_echo(context, inbuf, outbuf);
  test_streaming_echo(context, inbuf, outbuf);
  test_streaming_censored_echo(context, inbuf, outbuf);
  test_streaming_output_error(context, inbuf, outbuf);
  test_streaming_input_error(context, inbuf, outbuf);
  test_streaming_multiple_errors(context, inbuf, outbuf);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}

auto censored_echo_method_factory(std::string censored)
{
  return [ censored = std::move(censored) ](
    result_t<void>& result,
    logging_context_t const& context,
    bound_inbuf_t& inbuf,
    bound_outbuf_t& outbuf)
  {
    return make_method<echo_handler_t>(
      result, context, inbuf, outbuf, censored);
  };
}
    
void do_run_tests(logging_context_t const& client_context,
                  logging_context_t const& server_context,
                  std::size_t bufsize)
{
  if(auto msg = client_context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting; bufsize: " << bufsize;
  }

  method_map_t map;
  map.add_method_factory(
    "add", default_method_factory<add_handler_t>());
  map.add_method_factory(
    "censored_echo", censored_echo_method_factory(censored));
  map.add_method_factory(
    "echo", default_method_factory<echo_handler_t>());
  map.add_method_factory(
    "subtract", default_method_factory<subtract_handler_t>());

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

  std::size_t constexpr bufsizes[] =
    { 1, 1024, nb_inbuf_t::default_bufsize };
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
