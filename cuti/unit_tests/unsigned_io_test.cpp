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

#include <cuti/unsigned_reader.hpp>

#include <cuti/cmdline_reader.hpp>
#include <cuti/default_scheduler.hpp>
#include <cuti/eof_reader.hpp>
#include <cuti/final_result.hpp>
#include <cuti/logger.hpp>
#include <cuti/logging_context.hpp>
#include <cuti/nb_string_inbuf.hpp>
#include <cuti/option_walker.hpp>
#include <cuti/quoted_string.hpp>
#include <cuti/streambuf_backend.hpp>

#include <algorithm>
#include <iostream>
#include <limits>
#include <typeinfo>

#undef NDEBUG
#include <cassert>

namespace // anonymous
{

using namespace cuti;

std::size_t constexpr bufsizes[] = { 1, nb_inbuf_t::default_bufsize };

template<typename T>
std::string times_ten(T value)
{
  auto result = std::to_string(value);
  if(result != "0")
  {
    result += '0';
  }
  return result;
}

template<typename T>
std::string plus_one(T value)
{
  auto result = std::to_string(value);

  bool carry = true;
  auto rit = result.rbegin();
  while(rit != result.rend() && carry && *rit >= '0' && *rit <= '9')
  {
    if(*rit == '9')
    {
      *rit = '0';
    }
    else
    {
      ++(*rit);
      carry = false;
    }
    ++rit;
  }
    
  if(carry)
  {
    result.insert(rit.base(), '1');
  }
    
  return result;
}

template<typename T>
void do_test_successful_read(logging_context_t& context,
                             T expected,
                             std::string input,
                             std::size_t bufsize)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << '<' << typeid(T).name() << '>' <<
      ": expected: " << expected <<
      " input: " << quoted_string(input) << " bufsize: " << bufsize;
  }

  default_scheduler_t scheduler;

  auto inbuf = make_nb_string_inbuf(context, std::move(input), bufsize);
  bound_inbuf_t bit(*inbuf, scheduler);

  final_result_t<T> value_result;
  unsigned_reader_t<T> value_reader(value_result, bit);
  value_reader.start();

  while(!value_result.available())
  {
    auto cb = scheduler.wait();
    assert(cb != nullptr);
    cb();
  }

  assert(value_result.value() == expected);
  
  final_result_t<no_value_t> eof_result;
  eof_reader_t eof_reader(eof_result, bit);
  eof_reader.start();

  while(!eof_result.available())
  {
    auto cb = scheduler.wait();
    assert(cb != nullptr);
    cb();
  }

  eof_result.value();
}

template<typename T>
void do_test_failing_read(logging_context_t& context,
                          std::string input,
                          std::size_t bufsize)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << '<' << typeid(T).name() << '>' <<
      " input: " << quoted_string(input) << " bufsize: " << bufsize;
  }

  default_scheduler_t scheduler;

  auto inbuf = make_nb_string_inbuf(context, std::move(input), bufsize);
  bound_inbuf_t bit(*inbuf, scheduler);

  final_result_t<T> result;
  unsigned_reader_t<T> reader(result, bit);
  reader.start();

  while(auto cb = scheduler.wait())
  {
    cb();
  }

  bool caught = false;
  try
  {
    result.value();
  }
  catch(std::exception const& ex)
  {
    if(auto msg = context.message_at(loglevel_t::info))
    {
      *msg << "caught exception: " << ex.what();
    }
    caught = true;
  }
  assert(caught);
}

template<typename T>
void test_successful_read(logging_context_t& context)
{
  static T constexpr values[] =
    { 0, 4711, std::numeric_limits<T>::max() };

  for(auto value : values)
  {
    for(auto bufsize : bufsizes)
    {
      do_test_successful_read<T>(context, value,
        std::to_string(value), bufsize);
      do_test_successful_read<T>(context, value,
        "\t" + std::to_string(value), bufsize);
      do_test_successful_read<T>(context, value,
        "\r" + std::to_string(value), bufsize);
      do_test_successful_read<T>(context, value,
        " " + std::to_string(value), bufsize);
      do_test_successful_read<T>(context, value,
        "\t\r " + std::to_string(value), bufsize);
    }
  }
}

template<typename T>
void test_digit_expected(logging_context_t& context)
{
  for(auto bufsize : bufsizes)
  {
    do_test_failing_read<T>(context, "", bufsize);
    do_test_failing_read<T>(context, "Hello world", bufsize);
    do_test_failing_read<T>(context, "\tHello world", bufsize);
    do_test_failing_read<T>(context, "\rHello world", bufsize);
    do_test_failing_read<T>(context, " Hello world", bufsize);
    do_test_failing_read<T>(context, "\t\r Hello world", bufsize);
  }
}

template<typename T>
void test_overflow(logging_context_t& context)
{
  for(auto bufsize : bufsizes)
  {
    do_test_failing_read<T>(context,
      times_ten(std::numeric_limits<T>::max()), bufsize);
    do_test_failing_read<T>(context,
      "\t" + times_ten(std::numeric_limits<T>::max()), bufsize);
    do_test_failing_read<T>(context,
      "\r" + times_ten(std::numeric_limits<T>::max()), bufsize);
    do_test_failing_read<T>(context,
      " " + times_ten(std::numeric_limits<T>::max()), bufsize);
    do_test_failing_read<T>(context,
      "\t\r " + times_ten(std::numeric_limits<T>::max()), bufsize);

    do_test_failing_read<T>(context,
      plus_one(std::numeric_limits<T>::max()), bufsize);
    do_test_failing_read<T>(context,
      "\t" + plus_one(std::numeric_limits<T>::max()), bufsize);
    do_test_failing_read<T>(context,
      "\r" + plus_one(std::numeric_limits<T>::max()), bufsize);
    do_test_failing_read<T>(context,
      " " + plus_one(std::numeric_limits<T>::max()), bufsize);
    do_test_failing_read<T>(context,
      "\t\r " + plus_one(std::numeric_limits<T>::max()), bufsize);
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
  
  test_successful_read<unsigned short>(context);
  test_successful_read<unsigned int>(context);
  test_successful_read<unsigned long>(context);
  test_successful_read<unsigned long long>(context);

  test_digit_expected<unsigned short>(context);
  test_digit_expected<unsigned int>(context);
  test_digit_expected<unsigned long>(context);
  test_digit_expected<unsigned long long>(context);

  test_overflow<unsigned short>(context);
  test_overflow<unsigned int>(context);
  test_overflow<unsigned long>(context);
  test_overflow<unsigned long long>(context);

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
