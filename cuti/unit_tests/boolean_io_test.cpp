/*
 * Copyright (C) 2021-2025 CodeShop B.V.
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
#include <cuti/option_walker.hpp>
#include <cuti/streambuf_backend.hpp>

#include <cuti/io_test_utils.hpp>

#include <iostream>

#undef NDEBUG
#include <cassert>

namespace // anonymous
{

using namespace cuti;
using namespace cuti::io_test_utils;

template<typename T>
void do_test_failing_reads(logging_context_t const& context,
                           std::size_t bufsize)
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
void do_test_roundtrips(logging_context_t const& context, std::size_t bufsize)
{
  test_roundtrip(context, bufsize, T(false));
  test_roundtrip(context, bufsize, T(true));
  test_roundtrip(context, bufsize, make_vector<T>());
}

void test_failing_reads(logging_context_t const& context, std::size_t bufsize)
{
  do_test_failing_reads<bool>(context, bufsize);
  do_test_failing_reads<flag_t>(context, bufsize);
}

void test_roundtrips(logging_context_t const& context, std::size_t bufsize)
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
