/*
 * Copyright (C) 2021-2024 CodeShop B.V.
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

#include <cuti/async_readers.hpp>
#include <cuti/async_writers.hpp>
#include <cuti/cmdline_reader.hpp>
#include <cuti/default_scheduler.hpp>
#include <cuti/final_result.hpp>
#include <cuti/flusher.hpp>
#include <cuti/logging_context.hpp>
#include <cuti/nb_tcp_buffers.hpp>
#include <cuti/option_walker.hpp>
#include <cuti/stack_marker.hpp>
#include <cuti/streambuf_backend.hpp>
#include <cuti/tcp_connection.hpp>

#include <iostream>

#undef NDEBUG
#include <cassert>

namespace // anonymous
{

using namespace cuti;

struct servant_t
{
  using result_value_t = void;

  servant_t(logging_context_t const& context,
            result_t<void>& result,
            bound_inbuf_t& inbuf,
            bound_outbuf_t& outbuf)
  : context_(context)
  , result_(result)
  , method_reader_(*this, &servant_t::on_bad_request, inbuf)
  , argument_reader_(*this, &servant_t::on_bad_request, inbuf)
  , reply_writer_(*this, result_, outbuf)
  , exception_writer_(*this, result_, outbuf)
  , flusher_(*this, result_, outbuf)
  { }

  servant_t(servant_t const&) = delete;
  servant_t& operator=(servant_t const&) = delete;
  
  void start(stack_marker_t& base_marker)
  {
    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "servant: " << __func__;
    }

    method_reader_.start(base_marker, &servant_t::on_method);
  }

private :
  void on_method(stack_marker_t& base_marker, identifier_t method)
  {
    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "servant: " << __func__ << ": method: " << method;
    }

    if(method != "echo")
    {
      exception_writer_.start(
        base_marker,
	&servant_t::start_flusher,
        remote_error_t("bad_method", method.as_string()));
      return;
    }

    argument_reader_.start(base_marker, &servant_t::on_argument);
  }

  void on_argument(stack_marker_t& base_marker, std::string argument)
  {
    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "servant: " << __func__ << ": argument: " << argument;
    }

    reply_writer_.start(
      base_marker, &servant_t::start_flusher, std::move(argument));
  }

  void on_bad_request(stack_marker_t& base_marker, std::exception_ptr exptr)
  {
    try
    {
      std::rethrow_exception(exptr);
    }
    catch(std::exception const& ex)
    {
      if(auto msg = context_.message_at(loglevel_t::info))
      {
        *msg << "servant: " << __func__ << ": ex: " << ex.what();
      }

      exception_writer_.start(
        base_marker,
        &servant_t::start_flusher,
        remote_error_t("bad_request", ex.what()));
    }
  }

  void start_flusher(stack_marker_t& base_marker)
  {
    if(auto msg = context_.message_at(loglevel_t::info))
    {
        *msg << "servant: " << __func__;
    }

    flusher_.start(base_marker, &servant_t::on_flushed);
  }

  void on_flushed(stack_marker_t& base_marker)
  {
    if(auto msg = context_.message_at(loglevel_t::info))
    {
        *msg << "servant: " << __func__;
    }

    result_.submit(base_marker);
  }

private :
  logging_context_t const& context_;
  result_t<void>& result_;
  subroutine_t<servant_t, reader_t<identifier_t>,
    failure_mode_t::handle_in_parent> method_reader_;
  subroutine_t<servant_t, reader_t<std::string>,
    failure_mode_t::handle_in_parent> argument_reader_;
  subroutine_t<servant_t, writer_t<std::string>> reply_writer_;
  subroutine_t<servant_t, exception_writer_t> exception_writer_;
  subroutine_t<servant_t, flusher_t> flusher_;
};

struct request_t
{
  using result_value_t = std::string;

  request_t(logging_context_t const& context,
            result_t<std::string>& result,
            bound_inbuf_t& inbuf,
            bound_outbuf_t& outbuf)
  : context_(context)
  , result_(result)
  , reply_reader_(*this, result_, inbuf)
  , method_writer_(*this, result_, outbuf)
  , argument_writer_(*this, result_, outbuf)
  , flusher_(*this, result_, outbuf)
  , argument_()
  { }

  void start(
    stack_marker_t& base_marker,identifier_t method, std::string argument)
  {
    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "request: " << __func__ << ": method: " << method <<
        " argument: " << argument;
    }

    argument_ = std::move(argument); 

    reply_reader_.start(base_marker, &request_t::on_reply);
    method_writer_.start(
      base_marker, &request_t::on_method_written, std::move(method));
  }
    
private :
  void on_method_written(stack_marker_t& base_marker)
  {
    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "request: " << __func__;
    }

    argument_writer_.start(
      base_marker, &request_t::on_argument_written, std::move(argument_));
  }

  void on_argument_written(stack_marker_t& base_marker)
  {
    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "request: " << __func__;
    }

    flusher_.start(base_marker, &request_t::on_flushed);
  }

  void on_flushed(stack_marker_t&)
  {
    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "request: " << __func__;
    }
  }

  void on_reply(stack_marker_t& base_marker, std::string reply)
  {
    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "request: " << __func__ << ": reply: " << reply;
    }

    result_.submit(base_marker, std::move(reply));
  }
  
private :
  logging_context_t const& context_;
  result_t<std::string>& result_;
  subroutine_t<request_t, reader_t<std::string>> reply_reader_;
  subroutine_t<request_t, writer_t<identifier_t>> method_writer_;
  subroutine_t<request_t, writer_t<std::string>> argument_writer_;
  subroutine_t<request_t, flusher_t> flusher_;

  std::string argument_;
};
  
void test_bad_method(logging_context_t const& context, std::size_t bufsize)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting (bufsize: " << bufsize << ')';
  }

  std::unique_ptr<tcp_connection_t> server_side;
  std::unique_ptr<tcp_connection_t> client_side;
  std::tie(server_side, client_side) = make_connected_pair();
  
  std::unique_ptr<nb_inbuf_t> server_in;
  std::unique_ptr<nb_outbuf_t> server_out;
  std::tie(server_in, server_out) =
    make_nb_tcp_buffers(std::move(server_side), bufsize, bufsize);

  std::unique_ptr<nb_inbuf_t> client_in;
  std::unique_ptr<nb_outbuf_t> client_out;
  std::tie(client_in, client_out) =
    make_nb_tcp_buffers(std::move(client_side), bufsize, bufsize);
  
  default_scheduler_t scheduler;

  bound_inbuf_t bound_server_in(*server_in, scheduler);
  bound_outbuf_t bound_server_out(*server_out, scheduler);
  
  bound_inbuf_t bound_client_in(*client_in, scheduler);
  bound_outbuf_t bound_client_out(*client_out, scheduler);
  
  stack_marker_t base_marker;

  final_result_t<void> servant_result;
  servant_t servant(context, servant_result,
    bound_server_in, bound_server_out);
  servant.start(base_marker);

  final_result_t<std::string> request_result;
  request_t request(context, request_result,
    bound_client_in, bound_client_out);
  request.start(base_marker, "tryme", "and see");

  unsigned int n_callbacks = 0;
  while(!request_result.available())
  {
    callback_t cb = scheduler.wait();
    assert(cb != nullptr);
    cb(base_marker);
    ++n_callbacks;
  }

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": n_callbacks: " << n_callbacks;
  }

  bool caught = false;
  try
  {
    request_result.value();
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

  std::size_t constexpr bufsizes[] = { 1, nb_inbuf_t::default_bufsize };
  for(auto bufsize : bufsizes)
  {
    test_bad_method(context, bufsize);
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
