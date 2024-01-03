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

#include "io_test_utils.hpp"

#include <cuti/async_readers.hpp>
#include <cuti/async_writers.hpp>

#include <cuti/cmdline_reader.hpp>
#include <cuti/option_walker.hpp>
#include <cuti/streambuf_backend.hpp>

#include <iostream>
#include <typeinfo>
#include <utility>

#undef NDEBUG
#include <cassert>

namespace // anoymous
{

using namespace cuti;
using namespace cuti::io_test_utils;

void test_failing_reads(logging_context_t const& context, std::size_t bufsize)
{
  // missing opening curly
  test_failing_read<std::tuple<>>(context, bufsize, "");
  test_failing_read<std::tuple<>>(context, bufsize, "\t\r ");
  test_failing_read<std::pair<int, int>>(context, bufsize, "");
  test_failing_read<std::pair<int, int>>(context, bufsize, "\t\r ");
  test_failing_read<std::array<int, 2>>(context, bufsize, "");
  test_failing_read<std::array<int, 2>>(context, bufsize, "\t\r ");

  // missing closing curly
  test_failing_read<std::tuple<>>(context, bufsize, "{");
  test_failing_read<std::tuple<>>(context, bufsize, "{ \n}");
  test_failing_read<std::tuple<int>>(context, bufsize, "{ 100 ");
  test_failing_read<std::tuple<int>>(context, bufsize, "{ 100\n}");
  test_failing_read<std::pair<int, int>>(context, bufsize, "{ 100 101 ");
  test_failing_read<std::pair<int, int>>(context, bufsize, "{ 100 101\n}");
  test_failing_read<std::array<int, 2>>(context, bufsize, "{ 100 101 ");
  test_failing_read<std::array<int, 2>>(context, bufsize, "{ 100 101\n}");

  // error in element
  test_failing_read<std::tuple<int>>(context, bufsize, "{ \"Hello world\" }");
  test_failing_read<std::tuple<int, std::string>>(context, bufsize, "{ 1 2 }");
  test_failing_read<std::pair<int, std::string>>(context, bufsize, "{ 1 2 }");
  test_failing_read<std::array<int, 2>>(context, bufsize, "{ 100 \"Hello\" }");
}

auto tuple_of_tuples()
{
  return std::make_tuple(
    std::make_tuple(42, std::string("Alice")),
    std::make_tuple(66, std::string("Bob"))
  );
}

auto marx_family()
{
  using person_t = std::tuple<std::string, std::string, int>;

  auto heinrich = person_t{"Heinrich", "Marx", 1777};
  auto henriette = person_t{"Henriette", "Presburg", 1788};
  auto karl = person_t{"Karl", "Marx", 1818 };

  using family_t = std::tuple<person_t, person_t, std::vector<person_t>>;

  return family_t{heinrich, henriette, { karl }};
}

auto reverse_marx_family()
{
  using person_t = std::tuple<int, std::string, std::string>;

  auto heinrich = person_t{1777, "Heinrich", "Marx"};
  auto henriette = person_t{1788, "Henriette", "Presburg"};
  auto karl = person_t{1818, "Karl", "Marx"};

  using family_t = std::tuple<std::vector<person_t>, person_t, person_t>;

  return family_t{{karl}, heinrich, henriette};
}

template<std::size_t N>
auto marx_families()
{
  using T = decltype(marx_family());
  std::vector<T> result;

  result.reserve(N);
  for(std::size_t i = 0; i != N; ++i)
  {
    result.push_back(marx_family());
  }

  return result;
}
    
void test_roundtrips(logging_context_t const& context, std::size_t bufsize)
{
  test_roundtrip(context, bufsize, std::tuple<>{});
  test_roundtrip(context, bufsize, std::tuple<int>{42});
  test_roundtrip(context, bufsize, std::tuple<int, int>{42, 4711});
  test_roundtrip(context, bufsize, std::pair<int, int>{42, 4711});
  test_roundtrip(context, bufsize, std::array<int, 2>{42, 4711});
  test_roundtrip(context, bufsize, std::tuple<int, std::string>{42, "Alice"});
  test_roundtrip(context, bufsize, std::pair<int, std::string>{42, "Alice"});

  test_roundtrip(context, bufsize, tuple_of_tuples());
  test_roundtrip(context, bufsize, marx_family());
  test_roundtrip(context, bufsize, reverse_marx_family());
  test_roundtrip(context, bufsize, marx_families<1000>());
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
