/*
 * Copyright (C) 2021-2024 CodeShop B.V.
 *
 * This file is part of the x264_es_utils library.
 *
 * The x264_es_utils library is free software: you can redistribute it
 * and/or modify it under the terms of version 2 of the GNU General
 * Public License as published by the Free Software Foundation.
 *
 * The x264_es_utils library is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See version 2 of the GNU General Public License for more details.
 *
 * You should have received a copy of version 2 of the GNU General
 * Public License along with the x264_es_utils library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "config.hpp"

#include "service.hpp"

#include <cuti/args_reader.hpp>
#include <cuti/cmdline_reader.hpp>
#include <cuti/config_file_reader.hpp>
#include <cuti/exception_builder.hpp>
#include <cuti/file_backend.hpp>
#include <cuti/option_walker.hpp>
#include <cuti/resolver.hpp>
#include <cuti/syslog_backend.hpp>

#include <cassert>
#include <fstream>

namespace // anonymous
{

std::vector<cuti::endpoint_t>
default_endpoints()
{
  return cuti::local_interfaces(11264);
}
  
char const* copyright_notice()
{
  return
R"(Copyright (C) 2021-2024 CodeShop B.V.

This program is free software. It comes with ABSOLUTELY NO WARRANTY
and is licensed to you under the terms of version 2 of the GNU General
Public License as published by the Free Software Foundation. Under
certain conditions, you may modify and/or redistribute this program;
see <http://www.gnu.org/licenses/> for details.)";
}

} // anonymous

namespace x264_es_utils
{

config_t::config_t(int argc, char const* const argv[])
: argv0_((assert(argc > 0), argv[0]))
#ifndef _WIN32
, daemon_(false)
#endif
, directory_()
, dry_run_(false)
, endpoints_()
, encoder_settings_()
#ifndef _WIN32
, group_()
#endif
, logfile_()
, loglevel_(default_loglevel)
, pidfile_()
, rotation_depth_(cuti::file_backend_t::default_rotation_depth)
, dispatcher_config_()
, size_limit_(cuti::file_backend_t::no_size_limit)
, syslog_(false)
, syslog_name_("")
#ifndef _WIN32
, umask_()
, user_()
#endif
{
  cuti::cmdline_reader_t cmdline_reader(argc, argv);
  read_options(cmdline_reader);
}

#ifndef _WIN32

bool config_t::run_as_daemon() const
{
  return bool(daemon_);
}

cuti::group_id_t const* config_t::group_id() const
{
  return group_ ? &(*group_) : nullptr;
}

cuti::user_id_t const* config_t::user_id() const
{
  return user_ ? &(*user_) : nullptr;
}

cuti::umask_t const* config_t::umask() const
{
  return umask_ ? &(*umask_) : nullptr;
}

#endif

char const* config_t::directory() const
{
  return directory_.empty() ? nullptr : directory_.c_str();
}

std::unique_ptr<cuti::logging_backend_t>
config_t::create_logging_backend() const
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

std::unique_ptr<cuti::pidfile_t> config_t::create_pidfile() const
{
  std::unique_ptr<cuti::pidfile_t> result = nullptr;

  if(!pidfile_.empty())
  {
    result = std::make_unique<cuti::pidfile_t>(pidfile_);
  }

  return result;
}

std::unique_ptr<cuti::service_t>
config_t::create_service(cuti::logging_context_t& context) const
{
  context.level(loglevel_);

  auto endpoints = endpoints_;
  if(endpoints.empty())
  {
    endpoints = default_endpoints();
  }

  auto result = std::make_unique<service_t>(
    context, dispatcher_config_, encoder_settings_, endpoints);
  if(dry_run_)
  {
    result.reset();
  }
  return result;
}

config_t::~config_t()
{
}

void config_t::read_options(cuti::args_reader_t& reader)
{
  read_options(reader, 0);
}

void config_t::read_options(cuti::args_reader_t& reader,
                            int config_file_depth)
{
  static auto constexpr max_config_file_depth = 20;
 
  std::string config_filename;

  cuti::option_walker_t walker(reader);
  while(!walker.done())
  {
    if(walker.match("--config", config_filename))
    {
      if(config_file_depth >= max_config_file_depth)
      {
        cuti::exception_builder_t<std::runtime_error> builder;
        builder << reader.current_origin() <<
          ": maximum config file depth (" << max_config_file_depth <<
          ") exceeded";
        builder.explode();
      }
        
      std::ifstream ifs(config_filename);
      if(!ifs)
      {
        cuti::exception_builder_t<std::runtime_error> builder;
        builder << reader.current_origin() <<
          ": can't open config file '" << config_filename << "'";
        builder.explode();
      }
        
      cuti::config_file_reader_t config_file_reader(
        config_filename, *ifs.rdbuf());
      read_options(config_file_reader, config_file_depth + 1);      
    }

    else if(walker.match("--logfile", logfile_))
    {
      syslog_ = false;
      syslog_name_.clear();
    }

    else if(walker.match("--syslog", syslog_))
    {
      logfile_.clear();
      syslog_name_.clear();
    }

    else if(walker.match("--syslog-name", syslog_name_))
    {
      logfile_.clear();
      syslog_ = false;
    }

    else if(
#ifndef _WIN32
      !walker.match("--daemon", daemon_) &&
#endif
      !walker.match("--directory", directory_) &&
      !walker.match("--dry-run", dry_run_) &&
      !walker.match("--endpoint", endpoints_) &&
      !walker.match("--deterministic", encoder_settings_.deterministic_) &&
      !walker.match("--preset", encoder_settings_.preset_) &&
      !walker.match("--tune", encoder_settings_.tune_) &&
#ifndef _WIN32
      !walker.match("--group", group_) &&
#endif
      !walker.match("--loglevel", loglevel_) &&
      !walker.match("--max-connections",
        dispatcher_config_.max_connections_) &&
      !walker.match("--max-threads",
        dispatcher_config_.max_thread_pool_size_) &&
      !walker.match("--pidfile", pidfile_) &&
      !walker.match("--rotation-depth", rotation_depth_) &&
      !walker.match("--selector",
        dispatcher_config_.selector_factory_) &&
      !walker.match("--size-limit", size_limit_) &&
#ifndef _WIN32
      !walker.match("--umask", umask_) &&
      !walker.match("--user", user_) &&
#endif
      true
    )
    {
      cuti::exception_builder_t<std::runtime_error> builder;
      builder << reader.current_origin() <<
        ": unknown option '" << reader.current_argument() << "'" << std::endl;
      print_usage(builder);
      builder.explode();
    }
  }

  if(!reader.at_end())
  {
    cuti::exception_builder_t<std::runtime_error> builder;
    builder << reader.current_origin() <<
      ": unexpected argument '" << reader.current_argument() << "'" <<
      std::endl;
    print_usage(builder);
    builder.explode();
  }
}

void config_t::print_usage(std::ostream& os)
{
  os << std::endl;
  os << "usage: " << argv0_ << " [<option> ...]" << std::endl;
  os << "options are:" << std::endl;
  os << "  --config <path>          " <<
    "insert options from file <path>" << std::endl;
#ifndef _WIN32
  os << "  --daemon                 " <<
    "run as daemon" << std::endl;
#endif
  os << "  --deterministic          " <<
    "use deterministic encoding" << std::endl;
  os << "  --directory <path>       " <<
    "change directory to <path> (default: no change)" << std::endl;
  os << "  --dry-run                " <<
    "initialize the service, but do not run it" << std::endl;
  os << "  --endpoint <port>@<ip>   " <<
    "add endpoint to listen on" << std::endl;
  os << "                             (defaults:";
  {
    auto defaults = default_endpoints();
    for(auto const& endpoint : defaults)
    {
      os << ' ' << endpoint;
    }
    os << ")" << std::endl;
  }
#ifndef _WIN32
  os << "  --group <group name>     " <<
    "run under <group name>'s group id" << std::endl;
#endif
  os << "  --logfile <path>         " <<
    "log to file <path>" << std::endl;
  os << "  --loglevel <level>       " <<
    "sets loglevel (default: " << 
    cuti::loglevel_string(default_loglevel) << ')' << std::endl;
  os << "  --max-connections <n>    " <<
    "sets max #connections (default: " <<
    cuti::dispatcher_config_t::default_max_connections() <<
    "; 0=unlimited) " << std::endl;
  os << "  --max-threads <n>        " <<
    "sets max #threads (default: " <<
    cuti::dispatcher_config_t::default_max_thread_pool_size() <<
    "; 0=unlimited) " << std::endl;
  os << "  --pidfile <path>         " <<
    "create PID file <path> (default: none)" << std::endl;
  os << "  --rotation-depth <depth> " << 
    "sets logfile rotation depth (default: " <<
    cuti::file_backend_t::default_rotation_depth << ')' << std::endl;
  os << "  --selector <type>        " <<
    "sets selector type (default: " <<
    cuti::dispatcher_config_t::default_selector_factory() << ")" << std::endl;
  os << "  --size-limit <limit>     " <<
    "sets logfile size limit (default: none)" << std::endl;
  os << "  --syslog                 " <<
    "log to system log as " << cuti::default_syslog_name(argv0_) <<
    std::endl;
  os << "  --syslog-name <name>     " <<
    "log to system log as <name>" << std::endl;
#ifndef _WIN32
  os << "  --umask <mask>           " <<
    "set umask (default: no change)" << std::endl;
  os << "  --user <user name>       " <<
    "run under <user name>'s user id" << std::endl;
#endif
  os << std::endl;
  os << copyright_notice() << std::endl;
}

} // x264_es_utils
