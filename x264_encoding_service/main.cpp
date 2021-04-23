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
#include "streambuf_backend.hpp"
#include "syslog_backend.hpp"

#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>

namespace // anonymous
{

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

char const* gplv2_notice()
{
  return
R"(This program is free software. It comes with ABSOLUTELY NO WARRANTY
and is licensed to you under the terms of version 2 of the GNU General
Public License as published by the Free Software Foundation. Under
certain conditions, you may modify and/or redistribute this program;
see <http://www.gnu.org/licenses/> for details.)";
}

struct options_t
{
  static const int default_n_messages_ = 100;

  explicit options_t(char const* argv0)
  : delay_(0)
  , logfile_("")
  , logfile_size_limit_(cuti::file_backend_t::no_size_limit_)
  , n_messages_(default_n_messages_)
  , rotation_depth_(cuti::file_backend_t::default_rotation_depth_)
  , service_name_(default_service_name(argv0))
  , syslog_(false)
  { }

  unsigned int delay_;
  std::string logfile_;
  unsigned int logfile_size_limit_;
  unsigned int n_messages_;
  unsigned int rotation_depth_;
  std::string service_name_;
  cuti::flag_t syslog_;
};

void read_options(options_t& options, cuti::option_walker_t& walker)
{
  while(!walker.done())
  {
    if(!walker.match("--delay", options.delay_) &&
       !walker.match("--logfile", options.logfile_) &&
       !walker.match("--logfile-size-limit", options.logfile_size_limit_) &&
       !walker.match("--n-messages", options.n_messages_) &&
       !walker.match("--rotation-depth", options.rotation_depth_) &&
       !walker.match("--service-name", options.service_name_) &&
       !walker.match("--syslog", options.syslog_))
    {
      break;
    }
  }
}

void print_usage(std::ostream& os, char const* argv0)
{
  os
    << '\n'
    << "x264_encoding_service (C) 2021 CodeShop B.V.\n"
    << '\n'
    << gplv2_notice() << '\n'
    << '\n'
    << "usage: " << argv0 << " [<option> ...]\n"
    << "\n"
    << "options are:\n"

    << "  --delay <milliseconds>       sets delay after each message\n"
    << "                                 (default: none)\n"

    << "  --logfile <name>             log to file\n"

    << "  --logfile-size-limit <size>  sets logfile size limit\n"
    << "                                 (default: none)\n"

    << "  --n-messages <count>         sets message count\n"
    << "                                 (default: "
    << options_t::default_n_messages_ << ")\n"

    << "  --rotation-depth <depth>     sets logfile rotation depth\n"
    << "                                 (default: "
    << cuti::file_backend_t::default_rotation_depth_ << ")\n"

    << "  --service-name <name>        sets service name\n"
    << "                                 (default: "
    << default_service_name(argv0) << ")\n"

    << "  --syslog                     log to system log\n"

    << std::endl;
}

int throwing_main(int argc, char const* const argv[])
{
  options_t options(argv[0]);
  cuti::option_walker_t walker(argc, argv);
  read_options(options, walker);

  if(!walker.done() || walker.next_index() != argc)
  {
    print_usage(std::cerr, argv[0]);
    return 1;
  }

  cuti::logger_t logger(argv[0]);
  if(!options.logfile_.empty())
  {
    logger.set_backend(std::make_unique<cuti::file_backend_t>(
      options.logfile_, options.logfile_size_limit_,
      options.rotation_depth_));
  }
  else if(options.syslog_)
  {
    logger.set_backend(std::make_unique<cuti::syslog_backend_t>(
      options.service_name_.c_str()));
  }
  else
  {
    logger.set_backend(std::make_unique<cuti::streambuf_backend_t>(std::cerr));
  }

  auto loglevel = cuti::loglevel_t::error;
  for(unsigned int i = 0; i != options.n_messages_; ++i)
  {
    logger.report(loglevel, "logging message #" + std::to_string(i));

    switch(loglevel)
    {
    case cuti::loglevel_t::error :
      loglevel = cuti::loglevel_t::warning; break;
    case cuti::loglevel_t::warning :
      loglevel = cuti::loglevel_t::info; break;
    case cuti::loglevel_t::info :
      loglevel = cuti::loglevel_t::debug; break;
    default :
      loglevel = cuti::loglevel_t::error; break;
    }

    if(options.delay_ != 0)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(options.delay_));
    }
  }

  logger.report(cuti::loglevel_t::info, "exiting...");

  return 0;
}

} // anonymous

int main(int argc, char* argv[])
{
  int result = 1;

  try
  {
    result = throwing_main(argc, argv);
  }
  catch(std::exception const& ex)
  {
    std::cerr << argv[0] << ": exception: " << ex.what() << std::endl;
  }

  return result;
}
