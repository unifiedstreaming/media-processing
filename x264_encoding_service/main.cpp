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

#include "file_backend.hpp"
#include "logger.hpp"
#include "option_walker.hpp"

#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace xes
{

namespace // anonymous
{

struct options_t
{
  explicit options_t(char const* argv0);
  
  std::string logfile_;
  unsigned int logfile_size_limit_;
  unsigned int rotation_depth_;

  std::string service_name_;
  bool syslog_;
};

std::string default_service_name(char const* argv0)
{
  char const* last_segment = argv0;
  char const* last_dot = nullptr;

  char const *p;
  for(p = argv0; *p != '\0'; ++p)
  {
    switch(*p)
    {
    case '/' :
#ifdef _WIN32
    case ':' :
    case '\\' :
#endif
      last_segment = p + 1;
      last_dot = nullptr;
      break;
    case '.' :
      last_dot = p;
      break;
    default :
      break;
    }
  }

  if(last_dot == nullptr)
  {
    last_dot = p;
  }

  return std::string(last_segment, last_dot);
}

options_t::options_t(char const* argv0)
: logfile_("")
, logfile_size_limit_(xes::file_backend_t::no_size_limit_)
, rotation_depth_(file_backend_t::default_rotation_depth_)
, service_name_(default_service_name(argv0))
, syslog_(false)
{ }
  
unsigned int unsigned_option_value(char const* value, char const* option)
{
  static const auto max = std::numeric_limits<unsigned int>::max();
  
  unsigned int result = 0;

  do
  {
    if(*value < '0' || *value > '9')
    {
      throw std::runtime_error(
        std::string("digit expected in option value for ") + option);
    }
    unsigned int digit = *value - '0';

    if(result > max / 10 || result * 10 > max - digit)
    {
      throw std::runtime_error(
        std::string("overflow in option value for ") + option);
    }
    result *= 10;
    result += digit;

    ++value;
  } while(*value != '\0');

  return result;
}

int read_options(options_t& options, int argc, char const* const argv[])
{
  option_walker_t walker(argc, argv);

  while(!walker.done())
  {
    char const* value;

    if((value = walker.match_value("--logfile")) != nullptr)
    {
      options.logfile_ = value;
    }
    else if((value = walker.match_value("--logfile-size-limit")) != nullptr)
    {
      options.logfile_size_limit_ =
        unsigned_option_value(value, "--logfile-size-limit");
    }
    else if((value = walker.match_value("--rotation-depth")) != nullptr)
    {
      options.rotation_depth_ =
        unsigned_option_value(value, "--rotation_depth");
    }
    else if((value = walker.match_value("--service-name")) != nullptr)
    {
      options.service_name_ = value;
    }
    else if(walker.match_flag("--syslog"))
    {
      options.syslog_ = true;
    }
    else
    {
      return -1;
    }
  }

  return walker.next_index();
}

void print_usage(std::ostream& os, char const* argv0)
{
  os <<
    "\n"
    "x264_encoding_service (C) 2021 CodeShop B.V.\n" <<
    "\n"
    "The x264_encoding service comes with ABSOLUTELY NO WARRANTY and is free\n"
    "software licensed to you under version 2 of the GNU General Public\n"
    "License. See <http://www.gnu.org/licenses/> for details.\n"
    "\n"
    "usage: " << argv0 << " [<option> ...]\n"
    "\n"
    "options are:\n"
    "  --logfile <name>             sets logfile name\n"
    "  --logfile-size-limit <size>  sets logfile size limit\n"
    "                                 (default: none)\n"
    "  --rotation-depth <depth>     sets logfile rotation depth\n"
    "                                 (default: " <<
      file_backend_t::default_rotation_depth_ << ")\n"
    "  --service-name <name>        sets service name\n"
    "  --syslog                     log to system log\n"
    "\n"
    << std::flush;
}

int throwing_main(int argc, char const* const argv[])
{
  options_t options(argv[0]);

  if(read_options(options, argc, argv) != argc)
  {
    print_usage(std::cerr, argv[0]);
    return 1;
  }

  return 0;
}
  
} // anonymous

} // xes

int main(int argc, char* argv[])
{
  int result = 1;

  try
  {
    result = xes::throwing_main(argc, argv);
  }
  catch(std::exception const& ex)
  {
    std::cerr << argv[0] << ": exception: " << ex.what() << std::endl;
    result = 1;
  }

  return result;
}
