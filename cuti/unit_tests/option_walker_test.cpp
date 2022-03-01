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

#include <cuti/cmdline_reader.hpp>
#include <cuti/option_walker.hpp>
#include <cuti/logger.hpp>

#include <cstring>
#include <exception>
#include <iostream>
#include <limits>
#include <string>
#include <type_traits>

// enable assert()
#undef NDEBUG
#include <cassert>

namespace // anonymous
{

using namespace cuti;

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

void no_options_no_args()
{
  char const* argv[] = { "command" };
  int argc = sizeof argv / sizeof argv[0];
  cmdline_reader_t reader(argc, argv);
  option_walker_t walker(reader);

  assert(walker.done());
  assert(reader.at_end());
}

void matching_flag()
{
  char const* argv[] = { "command", "--flag" };
  int argc = sizeof argv / sizeof argv[0];
  cmdline_reader_t reader(argc, argv);
  option_walker_t walker(reader);

  flag_t flag = false;

  while(!walker.done())
  {
    if(!walker.match("--flag", flag))
    {
      break;
    }
  }

  assert(walker.done());
  assert(flag);
  assert(reader.at_end());
}

void non_matching_flag()
{
  char const* argv[] = { "command", "--notflag" };
  int argc = sizeof argv / sizeof argv[0];
  cmdline_reader_t reader(argc, argv);
  option_walker_t walker(reader);

  flag_t flag = false;

  while(!walker.done())
  {
    if(!walker.match("--flag", flag))
    {
      break;
    }
  }

  assert(!walker.done());
  assert(!flag);
}

void underscore_matches_hyphen()
{
  char const* argv[] = { "command", "--flag_option" };
  int argc = sizeof argv / sizeof argv[0];
  cmdline_reader_t reader(argc, argv);
  option_walker_t walker(reader);

  flag_t flag = false;

  while(!walker.done())
  {
    if(!walker.match("--flag-option", flag))
    {
      break;
    }
  }

  assert(walker.done());
  assert(flag);
  assert(reader.at_end());
}

void hyphen_matches_underscore()
{
  char const* argv[] = { "command", "--flag-option" };
  int argc = sizeof argv / sizeof argv[0];
  cmdline_reader_t reader(argc, argv);
  option_walker_t walker(reader);

  flag_t flag = false;

  while(!walker.done())
  {
    if(!walker.match("--flag_option", flag))
    {
      break;
    }
  }

  assert(walker.done());
  assert(flag);
  assert(reader.at_end());
}

void multiple_flags()
{
  char const* argv[] = { "command", "--flag1", "--flag2" };
  int argc = sizeof argv / sizeof argv[0];
  cmdline_reader_t reader(argc, argv);
  option_walker_t walker(reader);

  flag_t flag1 = false;
  flag_t flag2 = false;

  while(!walker.done())
  {
    if(!walker.match("--flag1", flag1) &&
       !walker.match("--flag2", flag2))
    {
      break;
    }
  }

  assert(walker.done());
  assert(flag1);
  assert(flag2);
  assert(reader.at_end());
}

void value_assign()
{
  char const* argv[] = { "command", "--option=value" };
  int argc = sizeof argv / sizeof argv[0];
  cmdline_reader_t reader(argc, argv);
  option_walker_t walker(reader);

  std::string option;

  while(!walker.done())
  {
    if(!walker.match("--option", option))
    {
      break;
    }
  }

  assert(walker.done());
  assert(option == "value");
  assert(reader.at_end());
}

void value_separate()
{
  char const* argv[] = { "command", "--option", "value" };
  int argc = sizeof argv / sizeof argv[0];
  cmdline_reader_t reader(argc, argv);
  option_walker_t walker(reader);

  std::string option;

  while(!walker.done())
  {
    if(!walker.match("--option", option))
    {
      break;
    }
  }

  assert(walker.done());
  assert(option == "value");
  assert(reader.at_end());
}

void missing_value()
{
  char const* argv[] = { "command", "--option" } ;
  int argc = sizeof argv / sizeof argv[0];
  cmdline_reader_t reader(argc, argv);
  option_walker_t walker(reader);

  bool caught = false;
  try
  {
    std::string option;

    while(!walker.done())
    {
      if(!walker.match("--option", option))
      {
        break;
      }
    }
  }
  catch(std::exception const& /* ex */)
  {
    // std::cout << ex.what();
    caught = true;
  }

  assert(caught);
}

void two_values()
{
  char const* argv[] = { "command",
                         "--option1", "value1",
                         "--option2", "value2" };
  int argc = sizeof argv / sizeof argv[0];
  cmdline_reader_t reader(argc, argv);
  option_walker_t walker(reader);

  std::string option1;
  std::string option2;

  while(!walker.done())
  {
    if(!walker.match("--option1", option1) &&
       !walker.match("--option2", option2))
    {
      break;
    }
  }

  assert(walker.done());
  assert(option1 == "value1");
  assert(option2 == "value2");
  assert(reader.at_end());
}

void single_arg()
{
  char const* argv[] = { "command", "arg" };
  int argc = sizeof argv / sizeof argv[0];
  cmdline_reader_t reader(argc, argv);
  option_walker_t walker(reader);

  assert(walker.done());
  assert(!reader.at_end());
  assert(std::strcmp(reader.current_argument(), "arg") == 0);
}

void multiple_args()
{
  char const* argv[] = { "command", "arg1", "arg2" };
  int argc = sizeof argv / sizeof argv[0];
  cmdline_reader_t reader(argc, argv);
  option_walker_t walker(reader);

  assert(walker.done());
  assert(!reader.at_end());
  assert(std::strcmp(reader.current_argument(), "arg1") == 0);
}

void hyphens_at_start()
{
  char const* argv[] = { "command", "--", "--arg" };
  int argc = sizeof argv / sizeof argv[0];
  cmdline_reader_t reader(argc, argv);
  option_walker_t walker(reader);

  assert(walker.done());
  assert(!reader.at_end());
  assert(std::strcmp(reader.current_argument(), "--arg") == 0);
}

void hyphens_in_middle()
{
  char const* argv[] = { "command", "--flag", "--", "--arg" };
  int argc = sizeof argv / sizeof argv[0];
  cmdline_reader_t reader(argc, argv);
  option_walker_t walker(reader);

  flag_t flag = false;

  while(!walker.done())
  {
    if(!walker.match("--flag", flag))
    {
      break;
    }
  }

  assert(walker.done());
  assert(flag);
  assert(!reader.at_end());
  assert(std::strcmp(reader.current_argument(), "--arg") == 0);
}

void hyphens_at_end()
{
  char const* argv[] = { "command", "--flag1", "--flag2", "--" };
  int argc = sizeof argv / sizeof argv[0];
  cmdline_reader_t reader(argc, argv);
  option_walker_t walker(reader);

  flag_t flag1 = false;
  flag_t flag2 = false;

  while(!walker.done())
  {
    if(!walker.match("--flag1", flag1) &&
       !walker.match("--flag2", flag2))
    {
      break;
    }
  }

  assert(walker.done());
  assert(flag1);
  assert(flag2);
  assert(reader.at_end());
}

void single_short_flag()
{
  char const* argv[] = { "command", "-f" };
  int argc = sizeof argv / sizeof argv[0];
  cmdline_reader_t reader(argc, argv);
  option_walker_t walker(reader);

  flag_t flag = false;

  while(!walker.done())
  {
    if(!walker.match("-f", flag))
    {
      break;
    }
  }

  assert(walker.done());
  assert(flag);
  assert(reader.at_end());
}

void multiple_short_flags()
{
  char const* argv[] = { "command", "-f", "-g" };
  int argc = sizeof argv / sizeof argv[0];
  cmdline_reader_t reader(argc, argv);
  option_walker_t walker(reader);

  flag_t fflag = false;
  flag_t gflag = false;

  while(!walker.done())
  {
    if(!walker.match("-f", fflag) &&
       !walker.match("-g", gflag))
    {
      break;
    }
  }

  assert(walker.done());
  assert(fflag);
  assert(gflag);
  assert(reader.at_end());
}

void abbreviated_flags()
{
  char const* argv[] = { "command", "-fg" };
  int argc = sizeof argv / sizeof argv[0];
  cmdline_reader_t reader(argc, argv);
  option_walker_t walker(reader);

  flag_t fflag = false;
  flag_t gflag = false;

  while(!walker.done())
  {
    if(!walker.match("-f", fflag) &&
       !walker.match("-g", gflag))
    {
      break;
    }
  }

  assert(walker.done());
  assert(fflag);
  assert(gflag);
  assert(reader.at_end());
}

void short_value()
{
  char const* argv[] = { "command", "-o", "value" };
  int argc = sizeof argv / sizeof argv[0];
  cmdline_reader_t reader(argc, argv);
  option_walker_t walker(reader);

  std::string option;

  while(!walker.done())
  {
    if(!walker.match("-o", option))
    {
      break;
    }
  }

  assert(walker.done());
  assert(option == "value");
  assert(reader.at_end());
}

void value_in_abbreviation()
{
  char const* argv[] = { "command", "-fo", "value" };
  int argc = sizeof argv / sizeof argv[0];
  cmdline_reader_t reader(argc, argv);
  option_walker_t walker(reader);

  flag_t flag;
  std::string option;

  while(!walker.done())
  {
    if(!walker.match("-f", flag) &&
       !walker.match("-o", option))
    {
      break;
    }
  }

  assert(!walker.done());
  assert(flag);
  assert(option.empty());
}

void short_value_assign()
{
  char const* argv[] = { "command", "-o=value" };
  int argc = sizeof argv / sizeof argv[0];
  cmdline_reader_t reader(argc, argv);
  option_walker_t walker(reader);

  std::string option;

  while(!walker.done())
  {
    if(!walker.match("-o", option))
    {
      break;
    }
  }

  assert(option == "value");
  assert(reader.at_end());
}

void missing_short_value()
{
  char const* argv[] = { "command", "-o" };
  int argc = sizeof argv / sizeof argv[0];
  cmdline_reader_t reader(argc, argv);
  option_walker_t walker(reader);

  bool caught = false;
  try
  {
    std::string option;

    while(!walker.done())
    {
      if(!walker.match("-o", option))
      {
        break;
      }
    }
  }
  catch(std::exception const& /* ex */)
  {
    // std::cout << ex.what();
    caught = true;
  }

  assert(caught);
}

template<typename T>
void signed_option()
{
  static_assert(std::is_signed_v<T>);
  static_assert(std::is_integral_v<T>);
  
  char const* argv[] = { "command", "--number", "42" };
  int argc = sizeof argv / sizeof argv[0];
  cmdline_reader_t reader(argc, argv);
  option_walker_t walker(reader);

  T number = 0;

  while(!walker.done())
  {
    if(!walker.match("--number", number))
    {
      break;
    }
  }

  assert(walker.done());
  assert(number == 42);
  assert(reader.at_end());
}

template<typename T>
void negative_signed_option()
{
  static_assert(std::is_signed_v<T>);
  static_assert(std::is_integral_v<T>);
  
  char const* argv[] = { "command", "--number", "-42" };
  int argc = sizeof argv / sizeof argv[0];
  cmdline_reader_t reader(argc, argv);
  option_walker_t walker(reader);

  T number = 0;

  while(!walker.done())
  {
    if(!walker.match("--number", number))
    {
      break;
    }
  }

  assert(walker.done());
  assert(number == -42);
  assert(reader.at_end());
}

template<typename T>
void signed_option_overflow()
{
  static_assert(std::is_signed_v<T>);
  static_assert(std::is_integral_v<T>);
  
  T limit = std::numeric_limits<T>::max();
  std::string too_much = plus_one(limit);

  char const* argv[] = { "command", "--number", too_much.c_str() };
  int argc = sizeof argv / sizeof argv[0];
  cmdline_reader_t reader(argc, argv);
  option_walker_t walker(reader);

  bool caught = false;
  try
  {
    T number = 0;
    walker.match("--number", number);
  }
  catch(std::exception const& /* ex */)
  {
    // std::cout << ex.what() << std::endl;
    caught = true;
  }
  assert(caught);
}

template<typename T>
void signed_option_underflow()
{
  static_assert(std::is_signed_v<T>);
  static_assert(std::is_integral_v<T>);
  
  T limit = std::numeric_limits<T>::min();
  std::string too_little = plus_one(limit);

  char const* argv[] = { "command", "--number", too_little.c_str() };
  int argc = sizeof argv / sizeof argv[0];
  cmdline_reader_t reader(argc, argv);
  option_walker_t walker(reader);

  bool caught = false;
  try
  {
    T number = 0;
    walker.match("--number", number);
  }
  catch(std::exception const& /* ex */)
  {
    // std::cout << ex.what() << std::endl;
    caught = true;
  }
  assert(caught);
}

template<typename T>
void unsigned_option()
{
  static_assert(std::is_unsigned_v<T>);

  T value = std::numeric_limits<T>::max();
  std::string value_string = std::to_string(value);

  char const* argv[] = { "command", "--number", value_string.c_str() };
  int argc = sizeof argv / sizeof argv[0];
  cmdline_reader_t reader(argc, argv);
  option_walker_t walker(reader);

  T number = 0;

  while(!walker.done())
  {
    if(!walker.match("--number", number))
    {
      break;
    }
  }

  assert(walker.done());
  assert(number == value);
  assert(reader.at_end());
}

template<typename T>
void unsigned_option_overflow()
{
  static_assert(std::is_unsigned_v<T>);

  T limit = std::numeric_limits<T>::max();
  std::string too_much = plus_one(limit);

  char const* argv[] = { "command", "--number", too_much.c_str() };
  int argc = sizeof argv / sizeof argv[0];
  cmdline_reader_t reader(argc, argv);
  option_walker_t walker(reader);

  bool caught = false;
  try
  {
    T number = 0;
    walker.match("--number", number);
  }
  catch(std::exception const& /* ex */)
  {
    // std::cout << ex.what() << std::endl;
    caught = true;
  }
  assert(caught);
}

void additional_type()
{
  char const* argv[] = { "command", "--loglevel", "info" };
  int argc = sizeof argv / sizeof argv[0];
  cmdline_reader_t reader(argc, argv);
  option_walker_t walker(reader);

  loglevel_t level = loglevel_t::error;

  while(!walker.done())
  {
    if(!walker.match("--loglevel", level))
    {
      break;
    }
  }

  assert(walker.done());
  assert(level == loglevel_t::info);
  assert(reader.at_end());
}

void bad_value_for_additional_type()
{
  char const* argv[] = { "command", "--loglevel", "unknown" };
  int argc = sizeof argv / sizeof argv[0];
  cmdline_reader_t reader(argc, argv);
  option_walker_t walker(reader);

  bool caught = false;
  try
  {
    loglevel_t level;
    walker.match("--loglevel", level);
  }
  catch(std::exception const& /* ex */)
  {
    // std::cout << ex.what() << std::endl;
    caught = true;
  }
  assert(caught);
}

void repeated_flag_option()
{
  char const* argv[] = { "command", "-vvv" };
  int argc = sizeof argv / sizeof argv[0];
  cmdline_reader_t reader(argc, argv);
  option_walker_t walker(reader);

  std::vector<flag_t> flags;
  while(!walker.done())
  {
    if(!walker.match("-v", flags))
    {
      break;
    }
  }

  assert(walker.done());
  assert(flags.size() == 3);
  for(auto flag : flags)
  {
    assert(flag);
    assert(flag == flag);
    assert(!(flag != flag));

    assert(flag == true);
    assert(true == flag);
    assert(!(flag != true));
    assert(!(true != flag));

  }
}

void repeated_value_option()
{
  char const* argv[] =
    { "command", "--file=file1", "--file=file2", "--file=file3" };
  int argc = sizeof argv / sizeof argv[0];
  cmdline_reader_t reader(argc, argv);
  option_walker_t walker(reader);

  std::vector<std::string> files;
  while(!walker.done())
  {
    if(!walker.match("--file", files))
    {
      break;
    }
  }

  assert(walker.done());
  assert(files.size() == 3);
  assert(files[0] == "file1");
  assert(files[1] == "file2");
  assert(files[2] == "file3");
}

void optional_option()
{
  char const* argv[] =
    { "command", "--number=42" };
  int argc = sizeof argv / sizeof argv[0];
  cmdline_reader_t reader(argc, argv);
  option_walker_t walker(reader);

  std::optional<int> opt_number = std::nullopt;
  while(!walker.done())
  {
    if(!walker.match("--number", opt_number))
    {
      break;
    }
  }

  assert(walker.done());
  assert(opt_number != std::nullopt);
  assert(*opt_number == 42);
}
  
void run_tests(int, char const* const*)
{
  no_options_no_args();
  matching_flag();
  non_matching_flag();
  underscore_matches_hyphen();
  hyphen_matches_underscore();
  multiple_flags();
  value_assign();
  value_separate();
  missing_value();
  two_values();
  single_arg();
  multiple_args();
  hyphens_at_start();
  hyphens_in_middle();
  hyphens_at_end();

  single_short_flag();
  multiple_short_flags();
  abbreviated_flags();
  short_value();
  value_in_abbreviation();
  short_value_assign();
  missing_short_value();

  signed_option<short>();
  signed_option<int>();
  signed_option<long>();
  signed_option<long long>();
  
  negative_signed_option<short>();
  negative_signed_option<int>();
  negative_signed_option<long>();
  negative_signed_option<long long>();

  signed_option_overflow<short>();
  signed_option_overflow<int>();
  signed_option_overflow<long>();
  signed_option_overflow<long long>();

  signed_option_underflow<short>();
  signed_option_underflow<int>();
  signed_option_underflow<long>();
  signed_option_underflow<long long>();

  unsigned_option<unsigned short>();
  unsigned_option<unsigned int>();
  unsigned_option<unsigned long>();
  unsigned_option<unsigned long long>();

  unsigned_option_overflow<unsigned short>();
  unsigned_option_overflow<unsigned int>();
  unsigned_option_overflow<unsigned long>();
  unsigned_option_overflow<unsigned long long>();

  additional_type();
  bad_value_for_additional_type();

  repeated_flag_option();
  repeated_value_option();
  optional_option();
}

} // anonymous

int main(int argc, char* argv[])
{
  try
  {
    run_tests(argc, argv);
  }
  catch(std::exception const& ex)
  {
    std::cerr << argv[0] << ": exception: " << ex.what() << std::endl;
    throw;
  }

  return 0;
}
