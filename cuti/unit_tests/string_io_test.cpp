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

#include <cuti/string_reader.hpp>
#include <cuti/writer.hpp>

#include <cuti/bound_inbuf.hpp>
#include <cuti/bound_outbuf.hpp>
#include <cuti/charclass.hpp>
#include <cuti/cmdline_reader.hpp>
#include <cuti/default_scheduler.hpp>
#include <cuti/eof_checker.hpp>
#include <cuti/final_result.hpp>
#include <cuti/flusher.hpp>
#include <cuti/logger.hpp>
#include <cuti/logging_context.hpp>
#include <cuti/nb_string_inbuf.hpp>
#include <cuti/nb_string_outbuf.hpp>
#include <cuti/option_walker.hpp>
#include <cuti/quoted_string.hpp>
#include <cuti/streambuf_backend.hpp>

#include <iostream>
#include <utility>

#undef NDEBUG
#include <cassert>

namespace // anoymous
{

using namespace cuti;

void test_failing_read(logging_context_t& context,
                       std::string input,
                       std::size_t bufsize)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": input: " << quoted_string(input) <<
      " bufsize: " << bufsize;
  }

  default_scheduler_t scheduler;
  stack_marker_t base_marker;

  auto inbuf = make_nb_string_inbuf(std::move(input), bufsize);
  bound_inbuf_t bit(base_marker, *inbuf, scheduler);

  final_result_t<std::string> read_result;
  reader_t<std::string> reader(read_result, bit);
  reader.start();

  unsigned int n_read_callbacks = 0;
  while(!read_result.available())
  {
    auto cb = scheduler.wait();
    assert(cb != nullptr);
    cb();
    ++n_read_callbacks;
  }

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": n_read_callbacks: " << n_read_callbacks;
  }

  bool caught = false;
  try
  {
    read_result.value();
  }
  catch(std::exception const& ex)
  {
    if(auto msg = context.message_at(loglevel_t::info))
    {
      *msg << __func__ << ": caught exception: " << ex.what();
    }
    caught = true;
  }

  assert(caught);
}

void test_roundtrip(logging_context_t& context,
                    std::string input,
                    std::size_t bufsize)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": input: " << quoted_string(input) <<
      " bufsize: " << bufsize;
  }

  default_scheduler_t scheduler;
  stack_marker_t base_marker;

  std::string serialized_form;

  auto outbuf = make_nb_string_outbuf(serialized_form, bufsize);
  bound_outbuf_t bot(base_marker, *outbuf, scheduler);

  final_result_t<void> write_result;
  writer_t<std::string> writer(write_result, bot);
  writer.start(input);

  unsigned int n_write_callbacks = 0;
  while(!write_result.available())
  {
    auto cb = scheduler.wait();
    assert(cb != nullptr);
    cb();
    ++n_write_callbacks;
  }

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": n_write_callbacks: " << n_write_callbacks;
  }

  write_result.value();

  final_result_t<void> flush_result;
  flusher_t flusher(flush_result, bot);
  flusher.start();

  unsigned int n_flush_callbacks = 0;
  while(!flush_result.available())
  {
    auto cb = scheduler.wait();
    assert(cb != nullptr);
    cb();
    ++n_flush_callbacks;
  }

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": n_flush_callbacks: " << n_flush_callbacks;
  }

  flush_result.value();

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": serialized form: " <<
      quoted_string(serialized_form);
  }
  
  auto inbuf = make_nb_string_inbuf(serialized_form, bufsize);
  bound_inbuf_t bit(base_marker, *inbuf, scheduler);

  final_result_t<std::string> read_result;
  reader_t<std::string> reader(read_result, bit);
  reader.start();

  unsigned int n_read_callbacks = 0;
  while(!read_result.available())
  {
    auto cb = scheduler.wait();
    assert(cb != nullptr);
    cb();
    ++n_read_callbacks;
  }

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": n_read_callbacks: " << n_read_callbacks;
  }

  assert(read_result.value() == input);

  final_result_t<void> eof_result;
  eof_checker_t eof_checker(eof_result, bit);
  eof_checker.start();

  unsigned int n_eof_callbacks = 0;
  while(!eof_result.available())
  {
    auto cb = scheduler.wait();
    assert(cb != nullptr);
    cb();
    ++n_eof_callbacks;
  }

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": n_eof_callbacks: " << n_eof_callbacks;
  }

  eof_result.value();
}

void test_failing_reads(logging_context_t& context, std::size_t bufsize)
{
  // missing opening double quote
  test_failing_read(context, "", bufsize);
  test_failing_read(context, "\t\r ", bufsize);

  // missing closing double quote
  test_failing_read(context, "\"", bufsize);
  test_failing_read(context, "\"\n\"", bufsize);
  test_failing_read(context, "\"Bonkers", bufsize);
  test_failing_read(context, "\"Bonkers\n", bufsize);

  // non-printable in string value
  test_failing_read(context, "\"Hello\tWorld\"", bufsize);
  test_failing_read(context, "\"G\xffs de Gabber\"", bufsize);

  // unknown escape sequence
  test_failing_read(context, "\"What\\0\"", bufsize);
  test_failing_read(context, "\"What\\?\"", bufsize);

  // hex digit expected
  test_failing_read(context, "\"\\x\"", bufsize);
  test_failing_read(context, "\"\\xg\"", bufsize);
  test_failing_read(context, "\"\\xa\"", bufsize);
}

std::string printables()
{
  std::string result;
  for(int i = 0; i != 256; ++i)
  {
    if(is_printable(i))
    {
      result += static_cast<char>(i);
    }
  }
  return result;
}

std::string non_printables()
{
  std::string result;
  for(int i = 0; i != 256; ++i)
  {
    if(!is_printable(i))
    {
      result += static_cast<char>(i);
    }
  }
  return result;
}

std::string all_characters()
{
  std::string result;
  for(int i = 0; i != 256; ++i)
  {
    result += static_cast<char>(i);
  }
  return result;
}

void test_roundtrips(logging_context_t& context, std::size_t bufsize)
{
  test_roundtrip(context, "", bufsize);
  test_roundtrip(context, printables(), bufsize);
  test_roundtrip(context, non_printables(), bufsize);
  test_roundtrip(context, all_characters(), bufsize);
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
    test_failing_reads(context, bufsize);
    test_roundtrips(context, bufsize);
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
