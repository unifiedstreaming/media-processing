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

#include "option_walker.hpp"
#include "logger.hpp"

#include <cstring>
#include <iostream>
#include <limits>

// enable assert()
#undef NDEBUG
#include <cassert>

namespace // anonymous
{

void no_options_no_args()
{
  char const* argv[] = { "command" };
  int argc = sizeof argv / sizeof argv[0];
  cuti::option_walker_t walker(argc, argv);

  assert(walker.done());
  assert(walker.next_index() == 1);
}

void matching_flag()
{
  char const* argv[] = { "command", "--flag" };
  int argc = sizeof argv / sizeof argv[0];
  cuti::option_walker_t walker(argc, argv);

  cuti::flag_t flag = false;

  while(!walker.done())
  {
    if(!walker.match("--flag", flag))
    {
      break;
    }
  }

  assert(walker.done());
  assert(flag);
  assert(walker.next_index() == 2);
}
  
void non_matching_flag()
{
  char const* argv[] = { "command", "--notflag" };
  int argc = sizeof argv / sizeof argv[0];
  cuti::option_walker_t walker(argc, argv);

  cuti::flag_t flag = false;

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
  cuti::option_walker_t walker(argc, argv);

  cuti::flag_t flag = false;

  while(!walker.done())
  {
    if(!walker.match("--flag-option", flag))
    {
      break;
    }
  }

  assert(walker.done());
  assert(flag);
  assert(walker.next_index() == 2);
}

void hyphen_matches_underscore()
{
  char const* argv[] = { "command", "--flag-option" };
  int argc = sizeof argv / sizeof argv[0];
  cuti::option_walker_t walker(argc, argv);

  cuti::flag_t flag = false;

  while(!walker.done())
  {
    if(!walker.match("--flag_option", flag))
    {
      break;
    }
  }

  assert(walker.done());
  assert(flag);
  assert(walker.next_index() == 2);
}

void multiple_flags()
{
  char const* argv[] = { "command", "--flag1", "--flag2" };
  int argc = sizeof argv / sizeof argv[0];
  cuti::option_walker_t walker(argc, argv);

  cuti::flag_t flag1 = false;
  cuti::flag_t flag2 = false;

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
  assert(walker.next_index() == 3);
}

void value_assign()
{
  char const* argv[] = { "command", "--option=value" };
  int argc = sizeof argv / sizeof argv[0];
  cuti::option_walker_t walker(argc, argv);

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
  assert(walker.next_index() == 2);
}

void value_separate()
{
  char const* argv[] = { "command", "--option", "value" };
  int argc = sizeof argv / sizeof argv[0];
  cuti::option_walker_t walker(argc, argv);

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
  assert(walker.next_index() == 3);
}

void missing_value()
{
  char const* argv[] = { "command", "--option" } ;
  int argc = sizeof argv / sizeof argv[0];
  cuti::option_walker_t walker(argc, argv);

  std::string option;

  while(!walker.done())
  {
    if(!walker.match("--option", option))
    {
      break;
    }
  }

  assert(!walker.done());
  assert(option.empty());
}

void two_values()
{
  char const* argv[] = { "command",
                         "--option1", "value1",
                         "--option2", "value2" };
  int argc = sizeof argv / sizeof argv[0];
  cuti::option_walker_t walker(argc, argv);

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
  assert(walker.next_index() == 5);
}

void single_arg()
{
  char const* argv[] = { "command", "arg" };
  int argc = sizeof argv / sizeof argv[0];
  cuti::option_walker_t walker(argc, argv);

  assert(walker.done());
  assert(walker.next_index() == 1);
}

void multiple_args()
{
  char const* argv[] = { "command", "arg1", "arg2" };
  int argc = sizeof argv / sizeof argv[0];
  cuti::option_walker_t walker(argc, argv);

  assert(walker.done());
  assert(walker.next_index() == 1);
}

void hyphens_at_start()
{
  char const* argv[] = { "command", "--", "--arg" };
  int argc = sizeof argv / sizeof argv[0];
  cuti::option_walker_t walker(argc, argv);

  assert(walker.done());
  assert(walker.next_index() == 2);
}
  
void hyphens_in_middle()
{
  char const* argv[] = { "command", "--flag", "--", "--arg" };
  int argc = sizeof argv / sizeof argv[0];
  cuti::option_walker_t walker(argc, argv);

  cuti::flag_t flag = false;

  while(!walker.done())
  {
    if(!walker.match("--flag", flag))
    {
      break;
    }
  }

  assert(walker.done());
  assert(flag);
  assert(walker.next_index() == 3);
}
  
void hyphens_at_end()
{
  char const* argv[] = { "command", "--flag1", "--flag2", "--" };
  int argc = sizeof argv / sizeof argv[0];
  cuti::option_walker_t walker(argc, argv);

  cuti::flag_t flag1 = false;
  cuti::flag_t flag2 = false;

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
  assert(walker.next_index() == 4);
}

void single_short_flag()
{
  char const* argv[] = { "command", "-f" };
  int argc = sizeof argv / sizeof argv[0];
  cuti::option_walker_t walker(argc, argv);

  cuti::flag_t flag = false;

  while(!walker.done())
  {
    if(!walker.match("-f", flag))
    {
      break;
    }
  }

  assert(walker.done());
  assert(flag);
  assert(walker.next_index() == 2);
}

void multiple_short_flags()
{
  char const* argv[] = { "command", "-f", "-g" };
  int argc = sizeof argv / sizeof argv[0];
  cuti::option_walker_t walker(argc, argv);

  cuti::flag_t fflag = false;
  cuti::flag_t gflag = false;

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
  assert(walker.next_index() == 3);
}

void abbreviated_flags()
{
  char const* argv[] = { "command", "-fg" };
  int argc = sizeof argv / sizeof argv[0];
  cuti::option_walker_t walker(argc, argv);

  cuti::flag_t fflag = false;
  cuti::flag_t gflag = false;

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
  assert(walker.next_index() == 2);
}

void short_value()
{
  char const* argv[] = { "command", "-o", "value" };
  int argc = sizeof argv / sizeof argv[0];
  cuti::option_walker_t walker(argc, argv);

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
  assert(walker.next_index() == 3);
}

void value_in_abbreviation()
{
  char const* argv[] = { "command", "-fo", "value" };
  int argc = sizeof argv / sizeof argv[0];
  cuti::option_walker_t walker(argc, argv);

  cuti::flag_t flag;
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
  cuti::option_walker_t walker(argc, argv);

  std::string option;

  while(!walker.done())
  {
    if(!walker.match("-o", option))
    {
      break;
    }
  }

  assert(option == "value");
  assert(walker.next_index() == 2);
}

void missing_short_value()
{
  char const* argv[] = { "command", "-o" };
  int argc = sizeof argv / sizeof argv[0];
  cuti::option_walker_t walker(argc, argv);

  std::string option;

  while(!walker.done())
  {
    if(!walker.match("-o", option))
    {
      break;
    }
  }

  assert(!walker.done());
  assert(option.empty());
}

void int_option()
{
  char const* argv[] = { "command", "--number", "42" };
  int argc = sizeof argv / sizeof argv[0];
  cuti::option_walker_t walker(argc, argv);

  int number = 0;

  while(!walker.done())
  {
    if(!walker.match("--number", number))
    {
      break;
    }
  }

  assert(walker.done());
  assert(number == 42);
  assert(walker.next_index() == 3);
}

void negative_int_option()
{
  char const* argv[] = { "command", "--number", "-42" };
  int argc = sizeof argv / sizeof argv[0];
  cuti::option_walker_t walker(argc, argv);

  int number = 0;

  while(!walker.done())
  {
    if(!walker.match("--number", number))
    {
      break;
    }
  }

  assert(walker.done());
  assert(number == -42);
  assert(walker.next_index() == 3);
}

void int_option_overflow()
{
  unsigned int limit = std::numeric_limits<int>::max();
  std::string too_much = std::to_string(limit + 1);

  char const* argv[] = { "command", "--number", too_much.c_str() };
  int argc = sizeof argv / sizeof argv[0];
  cuti::option_walker_t walker(argc, argv);

  bool caught = false;
  try
  {
    int number = 0;
    walker.match("--number", number);
  }
  catch(std::exception const& /* ex */)
  {
    // std::cout << ex.what() << std::endl;
    caught = true;
  }
  assert(caught);
}

void int_option_underflow()
{
  unsigned int limit = std::numeric_limits<int>::max();
  std::string too_little = "-" + std::to_string(limit + 2);

  char const* argv[] = { "command", "--number", too_little.c_str() };
  int argc = sizeof argv / sizeof argv[0];
  cuti::option_walker_t walker(argc, argv);

  bool caught = false;
  try
  {
    int number = 0;
    walker.match("--number", number);
  }
  catch(std::exception const& /* ex */)
  {
    // std::cout << ex.what() << std::endl;
    caught = true;
  }
  assert(caught);
}

void unsigned_int_option()
{
  unsigned int value = std::numeric_limits<unsigned int>::max();
  std::string value_string = std::to_string(value);
  
  char const* argv[] = { "command", "--number", value_string.c_str() };
  int argc = sizeof argv / sizeof argv[0];
  cuti::option_walker_t walker(argc, argv);

  unsigned int number = 0;

  while(!walker.done())
  {
    if(!walker.match("--number", number))
    {
      break;
    }
  }

  assert(walker.done());
  assert(number == value);
  assert(walker.next_index() == 3);
}

void additional_type()
{
  char const* argv[] = { "command", "--loglevel", "info" };
  int argc = sizeof argv / sizeof argv[0];
  cuti::option_walker_t walker(argc, argv);

  cuti::loglevel_t level = cuti::loglevel_t::error;

  while(!walker.done())
  {
    if(!walker.match("--loglevel", level))
    {
      break;
    }
  }

  assert(walker.done());
  assert(level == cuti::loglevel_t::info);
  assert(walker.next_index() == 3);
}

void bad_value_for_additional_type()
{
  char const* argv[] = { "command", "--loglevel", "unknown" };
  int argc = sizeof argv / sizeof argv[0];
  cuti::option_walker_t walker(argc, argv);

  bool caught = false;
  try
  {
    cuti::loglevel_t level;
    walker.match("--loglevel", level);
  }
  catch(std::exception const& /* ex */)
  {
    // std::cout << ex.what() << std::endl;
    caught = true;
  }
  assert(caught);
}

} // anonymous

int main()
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

  int_option();
  negative_int_option();
  int_option_overflow();
  int_option_underflow();
  unsigned_int_option();

  additional_type();
  bad_value_for_additional_type();

  return 0;
}
