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
#include <cuti/enum_mapping.hpp>
#include <cuti/exception_builder.hpp>
#include <cuti/io_test_utils.hpp>
#include <cuti/option_walker.hpp>
#include <cuti/parse_error.hpp>
#include <cuti/streambuf_backend.hpp>

#include <cstddef> // for std::byte
#include <exception>
#include <limits>
#include <iostream>
#include <string>
#include <type_traits>

#undef NDEBUG
#include <cassert>

namespace // anonymous
{

template<typename T>
struct enum_test_traits_t;

template<>
struct enum_test_traits_t<std::byte>
{
  static constexpr auto min_value =
    std::numeric_limits<unsigned char>::min();
  static constexpr auto max_value =
    std::numeric_limits<unsigned char>::max();
};

enum struct signed_char_enum_t : signed char
{
  minus_one = -1,
  zero,
  one
};

template<>
struct enum_test_traits_t<signed_char_enum_t>
{
  static constexpr auto min_value =
    cuti::to_underlying(signed_char_enum_t::minus_one);
  static constexpr auto max_value =
    cuti::to_underlying(signed_char_enum_t::one);
};

enum struct char_enum_t : char
{
  zero,
  one,
  two
};

template<>
struct enum_test_traits_t<char_enum_t>
{
  static constexpr auto min_value =
    cuti::to_underlying(char_enum_t::zero);
  static constexpr auto max_value =
    cuti::to_underlying(char_enum_t::two);
};

enum struct unsigned_char_enum_t : unsigned char
{
  zero,
  one,
  two
};

template<>
struct enum_test_traits_t<unsigned_char_enum_t>
{
  static constexpr auto min_value =
    cuti::to_underlying(unsigned_char_enum_t::zero);
  static constexpr auto max_value =
    cuti::to_underlying(unsigned_char_enum_t::two);
};

enum struct plain_enum_t
{
  minus_one = -1,
  zero,
  one
};

template<>
struct enum_test_traits_t<plain_enum_t>
{
  static constexpr auto min_value =
    cuti::to_underlying(plain_enum_t::minus_one);
  static constexpr auto max_value =
    cuti::to_underlying(plain_enum_t::one);
};

} // anonymous

template<>
struct cuti::enum_mapping_t<signed_char_enum_t>
{
  using value_t = std::underlying_type_t<signed_char_enum_t>;

  static
  signed_char_enum_t from_underlying(value_t value)
  {
    if(value < to_underlying(signed_char_enum_t::minus_one) ||
       value > to_underlying(signed_char_enum_t::one))
    {
      cuti::exception_builder_t<cuti::parse_error_t> builder;
      builder << "unexpected underlying value " << int(value) <<
        " for signed_char_enum_t";
      builder.explode();
    }

    return signed_char_enum_t{value};
  }
};

template<>
struct cuti::enum_mapping_t<char_enum_t>
{
  using value_t = std::underlying_type_t<char_enum_t>;

  static
  char_enum_t from_underlying(value_t value)
  {
    if(value < to_underlying(char_enum_t::zero) ||
       value > to_underlying(char_enum_t::two))
    {
      cuti::exception_builder_t<cuti::parse_error_t> builder;
      builder << "unexpected underlying value " << int(value) <<
        " for char_enum_t";
      builder.explode();
    }

    return char_enum_t{value};
  }
};

template<>
struct cuti::enum_mapping_t<unsigned_char_enum_t>
{
  using value_t = std::underlying_type_t<unsigned_char_enum_t>;

  static
  unsigned_char_enum_t from_underlying(value_t value)
  {
    if(value < to_underlying(unsigned_char_enum_t::zero) ||
       value > to_underlying(unsigned_char_enum_t::two))
    {
      cuti::exception_builder_t<cuti::parse_error_t> builder;
      builder << "unexpected underlying value " << int(value) <<
        " for unsigned_char_enum_t";
      builder.explode();
    }

    return unsigned_char_enum_t{value};
  }
};

template<>
struct cuti::enum_mapping_t<plain_enum_t>
{
  using value_t = std::underlying_type_t<plain_enum_t>;

  static
  plain_enum_t from_underlying(value_t value)
  {
    if(value < to_underlying(plain_enum_t::minus_one) ||
       value > to_underlying(plain_enum_t::one))
    {
      cuti::exception_builder_t<cuti::parse_error_t> builder;
      builder << "unexpected underlying value " << value <<
        " for plain_enum_t";
      builder.explode();
    }

    return plain_enum_t{value};
  }
};

namespace // anonymous
{

using namespace cuti;
using namespace cuti::io_test_utils;

constexpr auto max_byte_value =
  std::numeric_limits<std::underlying_type_t<std::byte>>::max();

template<typename EnumT>
void test_failing_enum_reads(
  logging_context_t const& context, std::size_t bufsize)
{
  test_failing_read<EnumT>(context, bufsize,
    std::to_string(enum_test_traits_t<EnumT>::min_value - 1) + " ");
  test_failing_read<EnumT>(context, bufsize,
    std::to_string(enum_test_traits_t<EnumT>::max_value + 1) + " ");
}
    
void test_failing_reads(logging_context_t const& context, std::size_t bufsize)
{
  test_failing_enum_reads<std::byte>(context, bufsize);
  test_failing_enum_reads<signed_char_enum_t>(context, bufsize);
  test_failing_enum_reads<char_enum_t>(context, bufsize);
  test_failing_enum_reads<unsigned_char_enum_t>(context, bufsize);
  test_failing_enum_reads<plain_enum_t>(context, bufsize);
}

template<typename EnumT>
void test_enum_roundtrips(
  logging_context_t const& context, std::size_t bufsize)
{
  test_roundtrip(context, bufsize,
    static_cast<EnumT>(enum_test_traits_t<EnumT>::min_value));
  test_roundtrip(context, bufsize,
    static_cast<EnumT>(enum_test_traits_t<EnumT>::max_value));
}
    
void test_roundtrips(logging_context_t const& context, std::size_t bufsize)
{
  test_enum_roundtrips<std::byte>(context, bufsize);
  test_enum_roundtrips<signed_char_enum_t>(context, bufsize);
  test_enum_roundtrips<char_enum_t>(context, bufsize);
  test_enum_roundtrips<unsigned_char_enum_t>(context, bufsize);
  test_enum_roundtrips<plain_enum_t>(context, bufsize);
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
