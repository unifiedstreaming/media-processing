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

#include <cuti/reader.hpp>
#include <cuti/writer.hpp>

#include <cuti/bound_inbuf.hpp>
#include <cuti/bound_outbuf.hpp>
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

#include <algorithm>
#include <iostream>
#include <limits>
#include <typeinfo>

#undef NDEBUG
#include <cassert>

namespace // anonymous
{

using namespace cuti;

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

  auto rit = result.rbegin();
  while(rit != result.rend() && *rit == '9')
  {
    *rit = '0';
    --rit;
  }

  if(rit != result.rend() && *rit >= '0' && *rit < '9')
  {
    ++(*rit);
  }
  else
  {
    result.insert(rit.base(), '1');
  }
    
  return result;
}

template<typename T>
void verify_input(logging_context_t& context,
                  default_scheduler_t& scheduler,
                  T expected,
                  std::string input,
                  std::size_t bufsize)
{
  stack_marker_t base_marker;
  auto inbuf = make_nb_string_inbuf(std::move(input), bufsize);
  bound_inbuf_t bit(base_marker, *inbuf, scheduler);

  final_result_t<T> value_result;
  reader_t<T> value_reader(value_result, bit);
  value_reader.start();

  while(!value_result.available())
  {
    auto cb = scheduler.wait();
    assert(cb != nullptr);
    cb();
  }

  assert(value_result.value() == expected);
  
  final_result_t<void> eof_result;
  eof_checker_t eof_checker(eof_result, bit);
  eof_checker.start();

  while(!eof_result.available())
  {
    auto cb = scheduler.wait();
    assert(cb != nullptr);
    cb();
  }

  eof_result.value();
}

template<typename T>
void do_test_successful_read(logging_context_t& context,
                             T expected,
                             std::string input,
                             std::size_t bufsize)
{
  default_scheduler_t scheduler;
  verify_input(context, scheduler, expected, std::move(input), bufsize);
}

template<typename T>
void do_test_failing_read(logging_context_t& context,
                          std::string input,
                          std::size_t bufsize)
{
  default_scheduler_t scheduler;
  stack_marker_t base_marker;

  auto inbuf = make_nb_string_inbuf(std::move(input), bufsize);
  bound_inbuf_t bit(base_marker, *inbuf, scheduler);

  final_result_t<T> result;
  reader_t<T> reader(result, bit);
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
void do_test_roundtrip(logging_context_t& context,
                       T value,
                       std::size_t bufsize)
{
  default_scheduler_t scheduler;
  std::string serialized;

  stack_marker_t base_marker;

  auto outbuf = make_nb_string_outbuf(serialized, bufsize);
  bound_outbuf_t bot(base_marker, *outbuf, scheduler);

  final_result_t<void> write_result;
  writer_t<T> value_writer(write_result, bot);
  value_writer.start(value);

  while(!write_result.available())
  {
    auto cb = scheduler.wait();
    assert(cb != nullptr);
    cb();
  }

  write_result.value();

  final_result_t<void> flush_result;
  flusher_t flusher(flush_result, bot);
  flusher.start();

  while(!flush_result.available())
  {
    auto cb = scheduler.wait();
    assert(cb != nullptr);
    cb();
  }

  flush_result.value();

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << "serialized form: " << quoted_string(serialized);
  }

  verify_input(context, scheduler, value, std::move(serialized), bufsize);
}

constexpr char const* prefixes[] = { "", "\t", "\r", " ", "\t\r " };

template<typename T>
std::vector<T> testing_values()
{
  if constexpr(std::is_unsigned_v<T>)
  {
    return std::vector<T>
    {
      0, 1, 9, 10, 11, 99, 100, 101, 4711,
      std::numeric_limits<T>::max()
    };
  }
  else
  {
    return std::vector<T>
    {
      std::numeric_limits<T>::min(),
      -4711, -101, -100, -99, -11, -10, -9, -1,
      0, 1, 9, 10, 11, 99, 100, 101, 4711,
      std::numeric_limits<T>::max()
    };
  }      
}

template<typename T>
void test_successful_read(logging_context_t& context, std::size_t bufsize)
{
  auto values = testing_values<T>();

  for(auto value : values)
  {
    for(auto const& prefix : prefixes)
    {
      std::string input = prefix + std::to_string(value);

      if(auto msg = context.message_at(loglevel_t::info))
      {
        *msg << __func__ << '<' << typeid(T).name() << '>' <<
          ": expected: " << value <<
          " input: " << quoted_string(input) << " bufsize: " << bufsize;
      }

      do_test_successful_read<T>(context, value, input, bufsize);
    }
  }
}

template<typename T>
std::vector<std::string> digit_expected_inputs()
{
  std::vector<std::string> suffixes{ "", "Hello world" };

  if constexpr(std::is_signed_v<T>)
  {
    std::vector<std::string> more_suffixes = suffixes;
    for(auto const& suffix: suffixes)
    {
      more_suffixes.push_back("-" + suffix);
    }
    suffixes = more_suffixes;
  }

  return suffixes;
}
    
template<typename T>
void test_digit_expected(logging_context_t& context, std::size_t bufsize)
{
  auto inputs = digit_expected_inputs<T>();
  for(auto prefix : prefixes)
  {
    for(auto const& suffix : inputs)
    {
      std::string input = prefix + suffix;

      if(auto msg = context.message_at(loglevel_t::info))
      {
        *msg << __func__ << '<' << typeid(T).name() << '>' <<
          " input: " << quoted_string(input) << " bufsize: " << bufsize;
      }

      do_test_failing_read<T>(context, input, bufsize);
    }
  }
}

template<typename T>
std::vector<std::string> overflow_inputs()
{
  std::vector<std::string> suffixes;
  suffixes.push_back(times_ten(std::numeric_limits<T>::max()));
  suffixes.push_back(plus_one(std::numeric_limits<T>::max()));

  if constexpr(std::is_signed_v<T>)
  {
    suffixes.push_back(times_ten(std::numeric_limits<T>::min()));
    suffixes.push_back(plus_one(std::numeric_limits<T>::min()));
  }

  return suffixes;
}
    
template<typename T>
void test_overflow(logging_context_t& context, std::size_t bufsize)
{
  auto inputs = overflow_inputs<T>();
  for(auto prefix : prefixes)
  {
    for(auto const& suffix : inputs)
    {
      auto input = prefix + suffix;

      if(auto msg = context.message_at(loglevel_t::info))
      {
        *msg << __func__ << '<' << typeid(T).name() << '>' <<
          " input: " << quoted_string(input) << " bufsize: " << bufsize;
      }

      do_test_failing_read<T>(context, input, bufsize);
    }
  }
}

template<typename T>
void test_roundtrip(logging_context_t& context, std::size_t bufsize)
{
  auto values = testing_values<T>();

  for(auto value : values)
  {

    if(auto msg = context.message_at(loglevel_t::info))
    {
      *msg << __func__ << '<' << typeid(T).name() << '>' <<
      ": value: " << value << " bufsize: " << bufsize;
    }

    do_test_roundtrip<T>(context, value, bufsize);
  }
}

template<typename T>
void run_tests_for(logging_context_t& context, std::size_t bufsize)
{
  test_successful_read<T>(context, bufsize);
  test_digit_expected<T>(context, bufsize);
  test_overflow<T>(context, bufsize);
  test_roundtrip<T>(context, bufsize);
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
  
  static std::size_t constexpr bufsizes[] = { 1, nb_inbuf_t::default_bufsize };

  for(auto bufsize : bufsizes)
  {
    run_tests_for<short>(context, bufsize);
    run_tests_for<int>(context, bufsize);
    run_tests_for<long>(context, bufsize);
    run_tests_for<long long>(context, bufsize);

    run_tests_for<unsigned short>(context, bufsize);
    run_tests_for<unsigned int>(context, bufsize);
    run_tests_for<unsigned long>(context, bufsize);
    run_tests_for<unsigned long long>(context, bufsize);
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
