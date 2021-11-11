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

#include <cuti/boolean_readers.cpp>
#include <cuti/boolean_writers.cpp>

#include <cuti/bound_inbuf.hpp>
#include <cuti/bound_outbuf.hpp>
#include <cuti/cmdline_reader.hpp>
#include <cuti/default_scheduler.hpp>
#include <cuti/eof_checker.hpp>
#include <cuti/final_result.hpp>
#include <cuti/flusher.hpp>
#include <cuti/loglevel.hpp>
#include <cuti/logging_context.hpp>
#include <cuti/option_walker.hpp>
#include <cuti/nb_string_inbuf.hpp>
#include <cuti/nb_string_outbuf.hpp>
#include <cuti/quoted_string.hpp>
#include <cuti/stack_marker.hpp>
#include <cuti/streambuf_backend.hpp>
#include <cuti/vector_reader.hpp>
#include <cuti/vector_writer.hpp>

#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <vector>

#undef NDEBUG
#include <cassert>

namespace // anonymous
{

using namespace cuti;

template<typename T>
void test_failing_read(logging_context_t& context, std::size_t bufsize,
                       std::string input)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << '<' << typeid(T).name() <<
      ">: bufsize: " << bufsize <<
      " input: " << quoted_string(input);
  }

  default_scheduler_t scheduler;
  stack_marker_t marker;

  auto inbuf = make_nb_string_inbuf(std::move(input), bufsize);
  bound_inbuf_t bit(marker, *inbuf, scheduler);

  final_result_t<T> read_result;
  reader_t<T> reader(read_result, bit);
  reader.start();

  std::size_t n_reading_callbacks = 0;
  while(!read_result.available())
  {
    auto cb = scheduler.wait();
    assert(cb != nullptr);
    cb();
    ++n_reading_callbacks;
  }

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << '<' << typeid(T).name() <<
      ">: n_reading_callbacks: " << n_reading_callbacks;
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
      *msg << __func__ << '<' << typeid(T).name() <<
        ">: caught exception: " << ex.what();
    }
    caught = true;
  }

  assert(caught);
}

template<typename T>
void test_roundtrip(logging_context_t& context, std::size_t bufsize, T value)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << '<' << typeid(T).name() <<
      ">: bufsize: " << bufsize;
  }

  default_scheduler_t scheduler;
  stack_marker_t marker;

  std::string serialized_form;
  auto outbuf = make_nb_string_outbuf(serialized_form, bufsize);
  bound_outbuf_t bot(marker, *outbuf, scheduler);

  final_result_t<void> write_result;
  writer_t<T> writer(write_result, bot);
  writer.start(value);

  std::size_t n_writing_callbacks = 0;
  while(!write_result.available())
  {
    auto cb = scheduler.wait();
    assert(cb != nullptr);
    cb();
    ++n_writing_callbacks;
  }

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << '<' << typeid(T).name() <<
      ">: n_writing_callbacks: " << n_writing_callbacks;
  }

  final_result_t<void> flush_result;
  flusher_t flusher(flush_result, bot);
  flusher.start();

  std::size_t n_flushing_callbacks = 0;
  while(!flush_result.available())
  {
    auto cb = scheduler.wait();
    assert(cb != nullptr);
    cb();
    ++n_flushing_callbacks;
  }

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << '<' << typeid(T).name() <<
      ">: n_flushing_callbacks: " << n_flushing_callbacks;
  }

  if(auto msg = context.message_at(loglevel_t::info))
  {
    auto size = serialized_form.size();

    *msg << __func__ << '<' << typeid(T).name() <<
      ">: serialized form (size: " << size << "): ";

    if(size <= 80)
    {
      *msg << quoted_string(serialized_form);
    }
    else
    {
      *msg << "<not printed>";
    }
  }

  auto inbuf = make_nb_string_inbuf(std::move(serialized_form), bufsize);

  bound_inbuf_t bit(marker, *inbuf, scheduler);

  final_result_t<T> read_result;
  reader_t<T> reader(read_result, bit);
  reader.start();

  std::size_t n_reading_callbacks = 0;
  while(!read_result.available())
  {
    auto cb = scheduler.wait();
    assert(cb != nullptr);
    cb();
    ++n_reading_callbacks;
  }

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << '<' << typeid(T).name() <<
      ">: n_reading_callbacks: " << n_reading_callbacks;
  }

  assert(read_result.value() == value);

  final_result_t<void> checker_result;
  eof_checker_t checker(checker_result, bit);
  checker.start();

  std::size_t n_checking_callbacks = 0;
  while(!checker_result.available())
  {
    auto cb = scheduler.wait();
    assert(cb != nullptr);
    cb();
    ++n_checking_callbacks;
  }

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << '<' << typeid(T).name() <<
      ">: n_checking_callbacks: " << n_checking_callbacks;
  }
}

template<typename T>
void do_test_failing_reads(logging_context_t& context, std::size_t bufsize)
{
  // unexpected eof
  test_failing_read<T>(context, bufsize, "");
  test_failing_read<T>(context, bufsize, "\t\r ");

  // unexpected eol
  test_failing_read<T>(context, bufsize, "\n*");
  test_failing_read<T>(context, bufsize, "\t\r \n*");

  // wrong type
  test_failing_read<T>(context, bufsize, "42");
  test_failing_read<T>(context, bufsize, "\t\r 42");
} 

template<typename T>
std::vector<T> make_vector()
{
  std::vector<T> result;
  result.reserve(100);

  for(int i = 0; i != 100; ++i)
  {
    result.push_back(i % 2 == 0);
  }

  return result;
}

template<typename T>
void do_test_roundtrips(logging_context_t& context, std::size_t bufsize)
{
  test_roundtrip(context, bufsize, T(false));
  test_roundtrip(context, bufsize, T(true));
  test_roundtrip(context, bufsize, make_vector<T>());
}

void test_failing_reads(logging_context_t& context, std::size_t bufsize)
{
  do_test_failing_reads<bool>(context, bufsize);
  do_test_failing_reads<flag_t>(context, bufsize);
}

void test_roundtrips(logging_context_t& context, std::size_t bufsize)
{
  do_test_roundtrips<bool>(context, bufsize);
  do_test_roundtrips<flag_t>(context, bufsize);
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
