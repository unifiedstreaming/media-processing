/*
 * Copyright (C) 2021 CodeShop B.V.
 *
 * This file is part of the x264_encoding_service.
 *
 * The x264_encoding_service is free software: you can redistribute it
 * and/or modify it under the terms of version 2 of the GNU General
 * Public License as published by the Free Software Foundation.
 *
 * The x264_encoding_service is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See version 2 of the GNU General Public License for more details.
 *
 * You should have received a copy of version 2 of the GNU General
 * Public License along with the x264_encoding_service.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "option_walker.hpp"
#include <cstring>

// enable assert()
#undef NDEBUG
#include <cassert>

namespace // anonymous
{

void no_options_no_args()
{
  char const* argv[] = { "command" };
  int argc = sizeof argv / sizeof argv[0];
  xes::option_walker_t walker(argc, argv);

  assert(walker.done());
  assert(walker.next_index() == 1);
}

void matching_flag()
{
  char const* argv[] = { "command", "--flag" };
  int argc = sizeof argv / sizeof argv[0];
  xes::option_walker_t walker(argc, argv);

  bool flag = false;

  while(!walker.done())
  {
    if(walker.match_flag("--flag"))
    {
      flag = true;
    }
    else
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
  xes::option_walker_t walker(argc, argv);

  bool flag = false;

  while(!walker.done())
  {
    if(walker.match_flag("--flag"))
    {
      flag = true;
    }
    else
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
  xes::option_walker_t walker(argc, argv);

  bool flag = false;

  while(!walker.done())
  {
    if(walker.match_flag("--flag-option"))
    {
      flag = true;
    }
    else
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
  xes::option_walker_t walker(argc, argv);

  bool flag = false;

  while(!walker.done())
  {
    if(walker.match_flag("--flag_option"))
    {
      flag = true;
    }
    else
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
  xes::option_walker_t walker(argc, argv);

  bool flag1 = false;
  bool flag2 = false;

  while(!walker.done())
  {
    if(walker.match_flag("--flag1"))
    {
      flag1 = true;
    }
    else if(walker.match_flag("--flag2"))
    {
      flag2 = true;
    }
    else
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
  xes::option_walker_t walker(argc, argv);

  char const* option = nullptr;

  while(!walker.done())
  {
    char const* value;
    
    if((value = walker.match_value("--option")) != nullptr)
    {
      option = value;
    }
    else
    {
      break;
    }
  }

  assert(walker.done());
  assert(option != nullptr);
  assert(std::strcmp(option, "value") == 0);
  assert(walker.next_index() == 2);
}

void value_separate()
{
  char const* argv[] = { "command", "--option", "value" };
  int argc = sizeof argv / sizeof argv[0];
  xes::option_walker_t walker(argc, argv);

  char const* option = nullptr;

  while(!walker.done())
  {
    char const* value;
    
    if((value = walker.match_value("--option")) != nullptr)
    {
      option = value;
    }
    else
    {
      break;
    }
  }

  assert(walker.done());
  assert(option != nullptr);
  assert(std::strcmp(option, "value") == 0);
  assert(walker.next_index() == 3);
}

void missing_value()
{
  char const* argv[] = { "command", "--option" } ;
  int argc = sizeof argv / sizeof argv[0];
  xes::option_walker_t walker(argc, argv);

  char const* option = nullptr;

  while(!walker.done())
  {
    char const* value;
    
    if((value = walker.match_value("--option")) != nullptr)
    {
      option = value;
    }
    else
    {
      break;
    }
  }

  assert(!walker.done());
  assert(option == nullptr);
}

void two_values()
{
  char const* argv[] = { "command",
                         "--option1", "value1",
                         "--option2", "value2" };
  int argc = sizeof argv / sizeof argv[0];
  xes::option_walker_t walker(argc, argv);

  char const* option1 = nullptr;
  char const* option2 = nullptr;

  while(!walker.done())
  {
    char const* value;
    
    if((value = walker.match_value("--option1")) != nullptr)
    {
      option1 = value;
    }
    else if((value = walker.match_value("--option2")) != nullptr)
    {
      option2 = value;
    }
    else
    {
      break;
    }
  }

  assert(walker.done());
  assert(option1 != nullptr);
  assert(std::strcmp(option1, "value1") == 0);
  assert(option2 != nullptr);
  assert(std::strcmp(option2, "value2") == 0);
  assert(walker.next_index() == 5);
}

void single_arg()
{
  char const* argv[] = { "command", "arg" };
  int argc = sizeof argv / sizeof argv[0];
  xes::option_walker_t walker(argc, argv);

  assert(walker.done());
  assert(walker.next_index() == 1);
}

void multiple_args()
{
  char const* argv[] = { "command", "arg1", "arg2" };
  int argc = sizeof argv / sizeof argv[0];
  xes::option_walker_t walker(argc, argv);

  assert(walker.done());
  assert(walker.next_index() == 1);
}

void hyphens_at_start()
{
  char const* argv[] = { "command", "--", "--arg" };
  int argc = sizeof argv / sizeof argv[0];
  xes::option_walker_t walker(argc, argv);

  assert(walker.done());
  assert(walker.next_index() == 2);
}
  
void hyphens_in_middle()
{
  char const* argv[] = { "command", "--flag", "--", "--arg" };
  int argc = sizeof argv / sizeof argv[0];
  xes::option_walker_t walker(argc, argv);

  bool flag = false;

  while(!walker.done())
  {
    if(walker.match_flag("--flag"))
    {
      flag = true;
    }
    else
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
  xes::option_walker_t walker(argc, argv);

  bool flag1 = false;
  bool flag2 = false;

  while(!walker.done())
  {
    if(walker.match_flag("--flag1"))
    {
      flag1 = true;
    }
    else if(walker.match_flag("--flag2"))
    {
      flag2 = true;
    }
    else
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
  xes::option_walker_t walker(argc, argv);

  bool flag = false;

  while(!walker.done())
  {
    if(walker.match_flag("-f"))
    {
      flag = true;
    }
    else
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
  xes::option_walker_t walker(argc, argv);

  bool fflag = false;
  bool gflag = false;

  while(!walker.done())
  {
    if(walker.match_flag("-f"))
    {
      fflag = true;
    }
    if(walker.match_flag("-g"))
    {
      gflag = true;
    }
    else
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
  xes::option_walker_t walker(argc, argv);

  bool fflag = false;
  bool gflag = false;

  while(!walker.done())
  {
    if(walker.match_flag("-f"))
    {
      fflag = true;
    }
    if(walker.match_flag("-g"))
    {
      gflag = true;
    }
    else
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
  xes::option_walker_t walker(argc, argv);

  char const* option = nullptr;

  while(!walker.done())
  {
    char const* value;

    if((value = walker.match_value("-o")) != nullptr)
    {
      option = value;
    }
    else
    {
      break;
    }
  }

  assert(walker.done());
  assert(option != nullptr);
  assert(std::strcmp(option, "value") == 0);
  assert(walker.next_index() == 3);
}

void value_in_abbreviation()
{
  char const* argv[] = { "command", "-fo", "value" };
  int argc = sizeof argv / sizeof argv[0];
  xes::option_walker_t walker(argc, argv);

  bool flag;
  char const* option = nullptr;

  while(!walker.done())
  {
    char const* value;

    if(walker.match_flag("-f"))
    {
      flag = true;
    }
    else if((value = walker.match_value("-o")) != nullptr)
    {
      option = value;
    }
    else
    {
      break;
    }
  }

  assert(!walker.done());
  assert(flag);
  assert(option == nullptr);
}

void short_value_assign()
{
  char const* argv[] = { "command", "-o=value" };
  int argc = sizeof argv / sizeof argv[0];
  xes::option_walker_t walker(argc, argv);

  char const* option = nullptr;

  while(!walker.done())
  {
    char const* value;

    if((value = walker.match_value("-o")) != nullptr)
    {
      option = value;
    }
    else
    {
      break;
    }
  }

  assert(walker.done());
  assert(option != nullptr);
  assert(std::strcmp(option, "value") == 0);
  assert(walker.next_index() == 2);
}

void missing_short_value()
{
  char const* argv[] = { "command", "-o" };
  int argc = sizeof argv / sizeof argv[0];
  xes::option_walker_t walker(argc, argv);

  char const* option = nullptr;

  while(!walker.done())
  {
    char const* value;

    if((value = walker.match_value("-o")) != nullptr)
    {
      option = value;
    }
    else
    {
      break;
    }
  }

  assert(!walker.done());
  assert(option == nullptr);
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

  return 0;
}
