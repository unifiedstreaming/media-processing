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

#include "exception_builder.hpp"
#include "file_backend.hpp"
#include "logger.hpp"
#include "option_walker.hpp"
#include "service.hpp"
#include "syslog_backend.hpp"

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace // anonymous
{

char const* gplv2_notice()
{
  return
R"(This program is free software. It comes with ABSOLUTELY NO WARRANTY
and is licensed to you under the terms of version 2 of the GNU General
Public License as published by the Free Software Foundation. Under
certain conditions, you may modify and/or redistribute this program;
see <http://www.gnu.org/licenses/> for details.)";
}

struct x264_encoding_service_t : cuti::service_t
{
  x264_encoding_service_t(cuti::logging_context_t& logging_context,
                          cuti::tcp_connection_t& control_connection)
  : logging_context_(logging_context)
  , control_connection_(control_connection)
  { }

  void run() override
  {
    if(auto msg = logging_context_.message_at(cuti::loglevel_t::info))
    {
      *msg << "Service started.";
    }

    char buf[1];
    char* next;
    do
    {
      control_connection_.read(buf, buf + 1, next);
    } while(next == nullptr);

    if(next == buf)
    {
      throw std::runtime_error("Unexpected EOF on control connection");
    }

    assert(next > buf);
    if(auto msg = logging_context_.message_at(cuti::loglevel_t::info))
    {
      int sig = buf[0];
      *msg << "Received signal " << sig << ". Stopping service.";
    }
  }
      
private :
  cuti::logging_context_t& logging_context_;
  cuti::tcp_connection_t& control_connection_;
};

struct x264_encoding_service_config_t : cuti::service_config_t
{
  x264_encoding_service_config_t(int argc, char const* const argv[])
  : argv0_((assert(argc > 0), argv[0])) 
#ifndef _WIN32
  , daemon_(false)
#endif
  , dry_run_(false)
  , logfile_()
  , loglevel_(default_loglevel)
  , pidfile_()
  , rotation_depth_(cuti::file_backend_t::default_rotation_depth)
  , size_limit_(cuti::file_backend_t::no_size_limit)
  , syslog_(false)
  , syslog_name_("")
  {
    cuti::option_walker_t walker(argc, argv);
    while(!walker.done())
    {
      if(
#ifndef _WIN32
        !walker.match("--daemon", daemon_) &&
#endif
        !walker.match("--dry-run", dry_run_) &&
        !walker.match("--logfile", logfile_) &&
        !walker.match("--loglevel", loglevel_) &&
        !walker.match("--pidfile", pidfile_) &&
        !walker.match("--rotation-depth", rotation_depth_) &&
        !walker.match("--size-limit", size_limit_) &&
        !walker.match("--syslog", syslog_) &&
        !walker.match("--syslog-name", syslog_name_))
      {
        cuti::exception_builder_t<std::runtime_error> builder;
        builder << "unknown option" << std::endl << std::endl;
        print_usage(builder);
        builder.explode();
      }
    }

    if(walker.next_index() != argc)
    {
      cuti::exception_builder_t<std::runtime_error> builder;
      builder << "unexpected argument" << std::endl << std::endl;
      print_usage(builder);
      builder.explode();
    }
  }

#ifndef _WIN32
  bool run_as_daemon() const override
  {
    return bool(daemon_);
  }
#endif

  std::unique_ptr<cuti::logging_backend_t>
  create_logging_backend() const override
  {
    std::unique_ptr<cuti::logging_backend_t> result = nullptr;

    if(!logfile_.empty())
    {
      result = std::make_unique<cuti::file_backend_t>(
        logfile_, size_limit_, rotation_depth_);
    }
    else if(!syslog_name_.empty())
    {
      result = std::make_unique<cuti::syslog_backend_t>(syslog_name_);
    }
    else if(syslog_)
    {
      result = std::make_unique<cuti::syslog_backend_t>(
        cuti::default_syslog_name(argv0_));
    }

    return result;
  }

  std::unique_ptr<cuti::pidfile_t> create_pidfile() const override
  {
    std::unique_ptr<cuti::pidfile_t> result = nullptr;

    if(!pidfile_.empty())
    {
      result = std::make_unique<cuti::pidfile_t>(pidfile_.c_str());
    }

    return result;
  }

  std::unique_ptr<cuti::service_t>
  create_service(cuti::logging_context_t& context,
                 cuti::tcp_connection_t& control_connection) const override
  {
    context.level(loglevel_);

    std::unique_ptr<cuti::service_t> result = nullptr;
    if(!dry_run_)
    {
      result = std::make_unique<x264_encoding_service_t>(
        context, control_connection);
    }
    return result;
  }

private :
  void print_usage(std::ostream& os)
  {
    os << "Copyright (C) 2021 CodeShop B.V." << std::endl;
    os << gplv2_notice() << std::endl << std::endl;

    os << "usage: " << argv0_ << " [<option> ...]" << std::endl;
    os << "options are:" << std::endl;
#ifndef _WIN32
    os << "  --daemon                 " <<
      "run as daemon" << std::endl;
#endif
    os << "  --dry-run                " <<
      "initialize the service, but do not run it" << std::endl;
    os << "  --logfile <path>         " <<
      "log to file <path>" << std::endl;
    os << "  --loglevel <level>       " <<
      "sets loglevel (default: " << 
      cuti::loglevel_string(default_loglevel) << ')' << std::endl;
    os << "  --pidfile <path>         " <<
      "create PID file <path> (default: none)" << std::endl;
    os << "  --rotation-depth <depth> " << 
      "sets logfile rotation depth (default: " <<
      cuti::file_backend_t::default_rotation_depth << ')' << std::endl;
    os << "  --size-limit <limit>     " <<
      "sets logfile size limit (default: none)" << std::endl;
    os << "  --syslog                 " <<
      "log to system log as " << cuti::default_syslog_name(argv0_) <<
      std::endl;
    os << "  --syslog-name <name>     " <<
      "log to system log as <name>" << std::endl;
  }

private :
  static constexpr cuti::loglevel_t default_loglevel = 
    cuti::loglevel_t::warning;

  std::string argv0_;
#ifndef _WIN32
  cuti::flag_t daemon_;
#endif
  cuti::flag_t dry_run_;
  std::string logfile_;
  cuti::loglevel_t loglevel_;
  std::string pidfile_;
  unsigned int rotation_depth_;
  unsigned int size_limit_;
  cuti::flag_t syslog_;  
  std::string syslog_name_;
};

struct x264_encoding_service_config_reader_t : cuti::service_config_reader_t
{
  std::unique_ptr<cuti::service_config_t>
  read_config(int argc, char const* const argv[]) const override
  {
    return std::make_unique<x264_encoding_service_config_t>(argc, argv);
  }
};
    
void throwing_main(int argc, char const* const argv[])
{
  x264_encoding_service_config_reader_t config_reader;
  cuti::run_service(config_reader, argc, argv);
}

} // anonymous

int main(int argc, char* argv[])
{
  int result = 1;

  try
  {
    throwing_main(argc, argv);
    result = 0;
  }
  catch(std::exception const& ex)
  {
    std::cerr << argv[0] << ": exception: " << ex.what() << std::endl;
  }

  return result;
}
