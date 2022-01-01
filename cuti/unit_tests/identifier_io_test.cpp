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

#include "io_test_utils.hpp"

#include <cuti/async_readers.hpp>
#include <cuti/async_writers.hpp>

#include <cuti/cmdline_reader.hpp>
#include <cuti/option_walker.hpp>
#include <cuti/streambuf_backend.hpp>

#undef NDEBUG
#include <cassert>

#include <iostream>

namespace { // anonymous

using namespace cuti;
using namespace cuti::io_test_utils;

constexpr char const *prefixes[] = { "", "\t", "\r", "\t\r " };

void test_failing_reads(logging_context_t& context, std::size_t bufsize)
{
  for(auto prefix : prefixes)
  {
    // missing leader
    test_failing_read<identifier_t>(context, bufsize,
      std::string(prefix) + "0_foo ");

    // unexpected eof
    test_failing_read<identifier_t>(context, bufsize,
      std::string(prefix) + "_3foo_3BAR");
  }
}

void test_roundtrips(logging_context_t& context, std::size_t bufsize)
{
  test_roundtrip(context, bufsize, identifier_t{"a"});
  test_roundtrip(context, bufsize, identifier_t{"A"});
  test_roundtrip(context, bufsize, identifier_t{"z"});
  test_roundtrip(context, bufsize, identifier_t{"Z"});
  test_roundtrip(context, bufsize, identifier_t{"_"});
  
  test_roundtrip(context, bufsize, identifier_t{"aa"});
  test_roundtrip(context, bufsize, identifier_t{"a42"});
  test_roundtrip(context, bufsize, identifier_t{"zz"});
  test_roundtrip(context, bufsize, identifier_t{"z42"});
  test_roundtrip(context, bufsize, identifier_t{"AA"});
  test_roundtrip(context, bufsize, identifier_t{"A42"});
  test_roundtrip(context, bufsize, identifier_t{"ZZ"});
  test_roundtrip(context, bufsize, identifier_t{"Z42"});
  test_roundtrip(context, bufsize, identifier_t{"__"});
  test_roundtrip(context, bufsize, identifier_t{"_42"});
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
