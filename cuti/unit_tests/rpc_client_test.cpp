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
#include <cuti/async_readers.hpp>
#include <cuti/cmdline_reader.hpp>
#include <cuti/default_scheduler.hpp>
#include <cuti/dispatcher.hpp>
#include <cuti/echo_handler.hpp>
#include <cuti/final_result.hpp>
#include <cuti/method_map.hpp>
#include <cuti/option_walker.hpp>
#include <cuti/quoted.hpp>
#include <cuti/resolver.hpp>
#include <cuti/rpc_client.hpp>
#include <cuti/scoped_thread.hpp>
#include <cuti/streambuf_backend.hpp>
#include <cuti/subtract_handler.hpp>
#include <cuti/tcp_connection.hpp>

#include <csignal>
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
      
template<typename... InputArgs, typename... OutputArgs>
void check_rpc_failure(logging_context_t const& context,
                       rpc_client_t& client,
                       identifier_t method,
                       input_list_t<InputArgs...>& input_args,
                       output_list_t<OutputArgs...>& output_args)
{
  bool caught = false;
  try
  {
    client(std::move(method), input_args, output_args);
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


void test_add(logging_context_t const& context, rpc_client_t& client)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  int reply{};
  auto input_args = make_input_list<int>(reply);

  auto output_args = make_output_list<int, int>(42, 4711);

  client("add", input_args, output_args);

  assert(reply == 4753);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}
  
void test_overflow(logging_context_t const& context, rpc_client_t& client)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  int reply{};
  auto input_args = make_input_list<int>(reply);

  auto output_args = make_output_list<int, int>(
    std::numeric_limits<int>::max(), 1);

  check_rpc_failure(context, client, "add", input_args, output_args);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}
  
void test_bad_method(logging_context_t const& context, rpc_client_t& client)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  int reply{};
  auto input_args = make_input_list<int>(reply);

  auto output_args = make_output_list<int, int>(42, 4711);

  check_rpc_failure(context, client, "huh", input_args, output_args);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}
  
void test_subtract(logging_context_t const& context, rpc_client_t& client)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  int reply{};
  auto input_args = make_input_list<int>(reply);

  auto output_args = make_output_list<int, int>(4753, 4711);

  client("subtract", input_args, output_args);

  assert(reply == 42);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}
  
void test_underflow(logging_context_t const& context, rpc_client_t& client)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  int reply{};
  auto input_args = make_input_list<int>(reply);

  auto output_args = make_output_list<int, int>(
    std::numeric_limits<int>::min(), 1);

  check_rpc_failure(context, client, "subtract", input_args, output_args);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}

void test_vector_echo(logging_context_t const& context, rpc_client_t& client)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  std::vector<std::string> reply;
  auto input_args = make_input_list<std::vector<std::string>>(reply);

  auto output_args = make_output_list<std::vector<std::string>>(echo_args);

  client("echo", input_args, output_args);

  assert(reply == echo_args);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}
  
void test_vector_censored_echo(logging_context_t const& context,
                               rpc_client_t& client)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  std::vector<std::string> reply;
  auto input_args = make_input_list<std::vector<std::string>>(reply);

  auto output_args = make_output_list<std::vector<std::string>>(echo_args);

  check_rpc_failure(context, client, "censored_echo", input_args, output_args);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}
  
void test_streaming_echo(logging_context_t const& context,
                         rpc_client_t& client)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  std::vector<std::string> reply;
  auto input_args = make_input_list<streaming_tag_t<std::string>>(
    string_sink_t(context, reply));

  auto output_args = make_output_list<streaming_tag_t<std::string>>(
    string_source_t(context));

  client("echo", input_args, output_args);

  assert(reply == echo_args);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}
  
void test_streaming_censored_echo(logging_context_t const& context,
                                  rpc_client_t& client)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  std::vector<std::string> reply;
  auto input_args = make_input_list<streaming_tag_t<std::string>>(
    string_sink_t(context, reply));

  auto output_args = make_output_list<streaming_tag_t<std::string>>(
    string_source_t(context));

  check_rpc_failure(context, client, "censored_echo", input_args, output_args);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}
  
void test_streaming_output_error(logging_context_t const& context,
                                 rpc_client_t& client)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  std::vector<std::string> reply;
  auto input_args = make_input_list<streaming_tag_t<std::string>>(
    string_sink_t(context, reply));

  auto output_args = make_output_list<streaming_tag_t<std::string>>(
    string_source_t(context, n_echo_args / 2));

  check_rpc_failure(context, client, "echo", input_args, output_args);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}
  
void test_streaming_input_error(logging_context_t const& context,
                                rpc_client_t& client)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  std::vector<std::string> reply;
  auto input_args = make_input_list<streaming_tag_t<std::string>>(
    string_sink_t(context, reply, n_echo_args / 2));

  auto output_args = make_output_list<streaming_tag_t<std::string>>(
    string_source_t(context));

  check_rpc_failure(context, client, "echo", input_args, output_args);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}
  
void test_streaming_multiple_errors(logging_context_t const& context,
                                    rpc_client_t& client)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  std::vector<std::string> reply;
  auto input_args = make_input_list<streaming_tag_t<std::string>>(
    string_sink_t(context, reply, n_echo_args / 4));

  auto output_args = make_output_list<streaming_tag_t<std::string>>(
    string_source_t(context, 3 * (n_echo_args / 4)));

  check_rpc_failure(context, client, "censored_echo", input_args, output_args);

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
    
void run_logic_tests(logging_context_t const& client_context,
                     logging_context_t const& server_context,
                     selector_factory_t factory,
                     std::size_t bufsize)
{
  if(auto msg = client_context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting; selector: " << factory <<
      " bufsize: " << bufsize;
  }

  dispatcher_config_t dispatcher_config;
  dispatcher_config.selector_factory_ = std::move(factory);
  dispatcher_config.bufsize_ = bufsize;

  method_map_t map;
    map.add_method_factory(
      "add", default_method_factory<add_handler_t>());
  map.add_method_factory(
      "censored_echo", censored_echo_method_factory(censored));
  map.add_method_factory(
      "echo", default_method_factory<echo_handler_t>());
  map.add_method_factory(
      "subtract", default_method_factory<subtract_handler_t>());

  {
    dispatcher_t dispatcher(server_context, dispatcher_config);
    endpoint_t server_endpoint = dispatcher.add_listener(
      local_interfaces(any_port).front(), map);
    rpc_client_t client(server_endpoint, bufsize, bufsize);

    {
      scoped_thread_t dispatcher_thread([&] { dispatcher.run(); });
      auto guard = make_scoped_guard([&] { dispatcher.stop(SIGINT); });

      test_add(client_context, client);
      test_overflow(client_context, client);
      test_bad_method(client_context, client);
      test_subtract(client_context, client);
      test_underflow(client_context, client);
      test_vector_echo(client_context, client);
      test_vector_censored_echo(client_context, client);
      test_streaming_echo(client_context, client);
      test_streaming_censored_echo(client_context, client);
      test_streaming_output_error(client_context, client);
      test_streaming_input_error(client_context, client);
      test_streaming_multiple_errors(client_context, client);
    }
  }

  if(auto msg = client_context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}
  
void throughput_echo_client(logging_context_t const& context,
                            endpoint_t const& endpoint,
                            std::size_t bufsize,
                            throughput_settings_t const& settings,
                            bool must_fail)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  rpc_client_t client(endpoint, bufsize, bufsize, settings);

  std::vector<std::string> reply;
  auto input_args = make_input_list<std::vector<std::string>>(reply);
  auto output_args = make_output_list<std::vector<std::string>>(echo_args);

  bool caught = false;
  try
  {
    client("echo", input_args, output_args);
  }
  catch(std::exception const& ex)
  {
    caught = true;

    if(auto msg = context.message_at(loglevel_t::info))
    {
      *msg << __func__ << ": caught expected exception: " << ex.what();
    }
  }

  if(must_fail)
  {
    assert(caught);
  }

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}

void test_throughput(logging_context_t const& client_context,
                     throughput_settings_t const& client_settings,
                     logging_context_t const& server_context,
                     throughput_settings_t const& server_settings,
                     selector_factory_t factory,
                     std::size_t bufsize,
                     bool must_fail)
{
  if(auto msg = client_context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting;" <<
      " selector: " << factory <<
      " client low ticks limit: " << client_settings.low_ticks_limit_ <<
      " server low ticks limit: " << server_settings.low_ticks_limit_ <<
      " bufsize: " << bufsize;
  }

  dispatcher_config_t dispatcher_config;
  dispatcher_config.selector_factory_ = std::move(factory);
  dispatcher_config.bufsize_ = bufsize;
  dispatcher_config.throughput_settings_ = server_settings;
  
  method_map_t map;
  map.add_method_factory("echo", default_method_factory<echo_handler_t>());

  {
    dispatcher_t dispatcher(server_context, dispatcher_config);
    endpoint_t endpoint = dispatcher.add_listener(
      local_interfaces(any_port).front(), map);
    {
      scoped_thread_t dispatcher_thread([&] { dispatcher.run(); });
      auto guard = make_scoped_guard([&] { dispatcher.stop(SIGINT); });

      throughput_echo_client(client_context, endpoint, bufsize,
        client_settings, must_fail);
    }
  }

  if(auto msg = client_context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}
  
void test_impatient_client(logging_context_t const& client_context,
                           logging_context_t const& server_context,
                           selector_factory_t factory,
                           std::size_t bufsize)
{
  if(auto msg = client_context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting; selector: " << factory <<
     " bufsize: " << bufsize;
  }

  throughput_settings_t client_settings;
  client_settings.low_ticks_limit_ = 0;
  client_settings.tick_length_ = milliseconds_t(1);

  throughput_settings_t server_settings;

  bool const must_fail = true;
  test_throughput(client_context, client_settings,
                  server_context, server_settings,
                  std::move(factory), bufsize, must_fail);

  if(auto msg = client_context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}
  
void test_impatient_server(logging_context_t const& client_context,
                           logging_context_t const& server_context,
                           selector_factory_t factory,
                           std::size_t bufsize)
{
  if(auto msg = client_context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting; selector: " << factory <<
     " bufsize: " << bufsize;
  }

  throughput_settings_t client_settings;

  throughput_settings_t server_settings;
  server_settings.low_ticks_limit_ = 0;
  server_settings.tick_length_ = milliseconds_t(1);

  /*
   * The server-side dispatcher will only enable throughput checking
   * after receiving the first chunk of data.  Therefore, even an
   * impatient server may succeed if the buffer size is big enough.
   */
  bool const must_fail = false;
  test_throughput(client_context, client_settings,
                  server_context, server_settings,
                  std::move(factory), bufsize, must_fail);

  if(auto msg = client_context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}
  
void test_impatient_client_and_server(logging_context_t const& client_context,
                                      logging_context_t const& server_context,
                                      selector_factory_t factory,
                                      std::size_t bufsize)
{
  if(auto msg = client_context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting; selector: " << factory <<
     " bufsize: " << bufsize;
  }

  throughput_settings_t client_settings;
  client_settings.low_ticks_limit_ = 0;
  client_settings.tick_length_ = milliseconds_t(1);

  throughput_settings_t server_settings;
  server_settings.low_ticks_limit_ = 0;
  server_settings.tick_length_ = milliseconds_t(1);

  bool const must_fail = true;
  test_throughput(client_context, client_settings,
                  server_context, server_settings,
                  std::move(factory), bufsize, must_fail);

  if(auto msg = client_context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}
  
void run_throughput_tests(logging_context_t const& client_context,
                          logging_context_t const& server_context,
                          selector_factory_t const& factory,
                          std::size_t bufsize)
{
  if(auto msg = client_context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting; bufsize: " << bufsize;
  }

  test_impatient_client(client_context, server_context, factory, bufsize);
  test_impatient_server(client_context, server_context, factory, bufsize);
  test_impatient_client_and_server(client_context, server_context,
    factory, bufsize);

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

  auto factories = available_selector_factories();
  for(auto const& factory : factories)
  {
    std::size_t constexpr bufsizes[] =
      { 1, 1024, nb_inbuf_t::default_bufsize };

    for(auto bufsize : bufsizes)
    {
      run_logic_tests(client_context, server_context, factory, bufsize);
    }
  
    for(auto bufsize : bufsizes)
    {
      run_throughput_tests(client_context, server_context, factory, bufsize);
    }
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
