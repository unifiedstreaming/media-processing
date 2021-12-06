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

#include <cuti/async_readers.hpp>
#include <cuti/async_writers.hpp>
#include <cuti/cmdline_reader.hpp>
#include <cuti/logging_context.hpp>
#include <cuti/option_walker.hpp>
#include <cuti/request_handler.hpp>
#include <cuti/streambuf_backend.hpp>

#include <iostream>

#undef NDEBUG
#include <cassert>

namespace // anonymous
{

using namespace cuti;

struct add_handler_t : method_handler_t
{
  add_handler_t(logging_context_t& context,
                result_t<void>& result,
                bound_inbuf_t& inbuf,
                bound_outbuf_t& outbuf)
  : context_(context)
  , result_(result)
  , int_reader_(*this, result, inbuf)
  , int_writer_(*this, result, outbuf)
  , first_arg_()
  { }

  void start()
  {
    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "add_handler: " << __func__;
    }

    int_reader_.start(&add_handler_t::on_first_arg);
  }

private :
  void on_first_arg(int arg)
  {
    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "add_handler: " << __func__ << ": arg: " << arg;
    }

    first_arg_ = arg;
    int_reader_.start(&add_handler_t::on_second_arg);
  }

  void on_second_arg(int arg)
  {
    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "add_handler: " << __func__ << ": arg: " << arg;
    }

    int_writer_.start(&add_handler_t::on_done, first_arg_ + arg);
  }

  void on_done()
  {
    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "add_handler: " << __func__;
    }

    result_.submit();
  }
    
private :
  logging_context_t& context_;
  result_t<void>& result_;
  subroutine_t<add_handler_t, reader_t<int>> int_reader_;
  subroutine_t<add_handler_t, writer_t<int>> int_writer_;

  int first_arg_;
};
  
struct subtract_handler_t : method_handler_t
{
  subtract_handler_t(logging_context_t& context,
                     result_t<void>& result,
                     bound_inbuf_t& inbuf,
                     bound_outbuf_t& outbuf)
  : context_(context)
  , result_(result)
  , int_reader_(*this, result, inbuf)
  , int_writer_(*this, result, outbuf)
  , first_arg_()
  { }

  void start()
  {
    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "subtract_handler: " << __func__;
    }

    int_reader_.start(&subtract_handler_t::on_first_arg);
  }

private :
  void on_first_arg(int arg)
  {
    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "subtract_handler: " << __func__ << ": arg: " << arg;
    }

    first_arg_ = arg;
    int_reader_.start(&subtract_handler_t::on_second_arg);
  }

  void on_second_arg(int arg)
  {
    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "subtract_handler: " << __func__ << ": arg: " << arg;
    }

    int_writer_.start(&subtract_handler_t::on_done, first_arg_ - arg);
  }

  void on_done()
  {
    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "subtract_handler: " << __func__;
    }

    result_.submit();
  }
    
private :
  logging_context_t& context_;
  result_t<void>& result_;
  subroutine_t<subtract_handler_t, reader_t<int>> int_reader_;
  subroutine_t<subtract_handler_t, writer_t<int>> int_writer_;

  int first_arg_;
};

template<typename T>
std::unique_ptr<T> make_handler(logging_context_t& context,
                                result_t<void>& result,
                                bound_inbuf_t& inbuf,
                                bound_outbuf_t& outbuf)
{
  return std::make_unique<T>(context, result, inbuf, outbuf);
}

method_entry_t const method_entries[] = {
  { "add", make_handler<add_handler_t> },
  { "subtract", make_handler<subtract_handler_t> }
};

method_map_t const method_map{method_entries};
  
void test_method_map()
{
  assert(method_map.find_method_entry(
    identifier_t("a")) == nullptr);
  assert(method_map.find_method_entry(
    identifier_t("add")) == &method_entries[0]);
  assert(method_map.find_method_entry(
    identifier_t("divide")) == nullptr);
  assert(method_map.find_method_entry(
    identifier_t("multiply")) == nullptr);
  assert(method_map.find_method_entry(
    identifier_t("subtract")) == &method_entries[1]);
  assert(method_map.find_method_entry(
    identifier_t("z")) == nullptr);
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

  test_method_map();

  std::size_t constexpr bufsizes[] = { 1, nb_inbuf_t::default_bufsize };
  for(auto bufsize : bufsizes)
  {
    // TODO: test request_handler_t
    static_cast<void>(bufsize);
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
