/*
 * Copyright (C) 2021-2025 CodeShop B.V.
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

#include <x264_proto/default_endpoints.hpp>

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

char const* copyright_notice()
{
  return
R"(Copyright (C) 2021-2025 CodeShop B.V.

This program is free software. It comes with ABSOLUTELY NO WARRANTY
and is licensed to you under the terms of version 2 of the GNU General
Public License as published by the Free Software Foundation. Under
certain conditions, you may modify and/or redistribute this program;
see <http://www.gnu.org/licenses/> for details.)";
}

} // anonymous

namespace x264_es_utils
{

config_t::config_t(cuti::socket_layer_t& sockets,
                   int argc, char const* const argv[])
: sockets_(sockets)
, argv0_((assert(argc > 0), argv[0]))
#ifndef _WIN32
, daemon_(false)
#endif
, directory_()
, dry_run_(false)
, endpoints_()
, encoder_settings_()
, logfile_()
, logfile_rotation_depth_(cuti::file_backend_t::default_rotation_depth)
, logfile_size_limit_(cuti::file_backend_t::no_size_limit)
, loglevel_(default_loglevel)
, pidfile_()
, dispatcher_config_()
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

cuti::user_t const* config_t::user() const
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
      logfile_, logfile_size_limit_, logfile_rotation_depth_);
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
    endpoints = x264_proto::default_endpoints(sockets_);
  }

  auto result = std::make_unique<service_t>(
    context, sockets_, dispatcher_config_, encoder_settings_, endpoints);
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
  static constexpr auto max_config_file_depth = 20;
 
  std::string config_filename;

  auto handle_endpoint = [this](char const* name,
                                cuti::args_reader_t const & reader,
                                char const *value)
  {
    cuti::endpoint_t ep;
    cuti::parse_endpoint(sockets_, name, reader, value, ep);
    this->endpoints_.push_back(ep);
  };
      

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
      !walker.match("--endpoint", handle_endpoint) &&
      !walker.match("--deterministic", encoder_settings_.deterministic_) &&
      !walker.match("--logfile-rotation-depth", logfile_rotation_depth_) &&
      !walker.match("--logfile-size-limit", logfile_size_limit_) &&
      !walker.match("--loglevel", loglevel_) &&
      !walker.match("--max-concurrent-requests",
        dispatcher_config_.max_concurrent_requests_) &&
      !walker.match("--max-connections",
        dispatcher_config_.max_connections_) &&
      !walker.match("--pidfile", pidfile_) &&
      !walker.match("--preset", encoder_settings_.preset_) &&
      !walker.match("--selector",
        dispatcher_config_.selector_factory_) &&
      !walker.match("--session-threads", encoder_settings_.session_threads_) &&
      !walker.match("--session-lookahead-threads",
        encoder_settings_.session_lookahead_threads_) &&
      !walker.match("--session-sliced-threads",
        encoder_settings_.session_sliced_threads_) &&
      !walker.match("--session-deterministic",
        encoder_settings_.session_deterministic_) &&
      !walker.match("--session-cpu-independent",
        encoder_settings_.session_cpu_independent_) &&
      !walker.match("--tune", encoder_settings_.tune_) &&
#ifndef _WIN32
      !walker.match("--umask", umask_) &&
      !walker.match("--user", user_) &&
#endif
      true
    )
    {
      cuti::exception_builder_t<std::runtime_error> builder;
      builder << reader.current_origin() <<
        ": unknown option '" << reader.current_argument() << "'";
      print_usage(builder);
      builder.explode();
    }
  }

  if(!reader.at_end())
  {
    cuti::exception_builder_t<std::runtime_error> builder;
    builder << reader.current_origin() <<
      ": unexpected argument '" << reader.current_argument() << "'";
    print_usage(builder);
    builder.explode();
  }
}

void config_t::print_usage(std::ostream& os)
{
  os << std::endl;
  os << "usage: " << argv0_ << " [<option> ...]" << std::endl;
  os << "options are:" << std::endl;
  os << "  --config <path>                  " <<
    "insert options from file <path>" << std::endl;
#ifndef _WIN32
  os << "  --daemon                         " <<
    "run as daemon" << std::endl;
#endif
  os << "  --deterministic                  " <<
    "use deterministic encoding" << std::endl;
  os << "  --directory <path>               " <<
    "change directory to <path>" << std::endl;
  os << "                                     (default: no change)" <<
    std::endl;
  os << "  --dry-run                        " <<
    "initialize the service, but do not run it" << std::endl;
  os << "  --endpoint <port>@<ip>           " <<
    "add endpoint to listen on" << std::endl;
  os << "                                     (defaults:";
  {
    auto defaults = x264_proto::default_endpoints(sockets_);
    for(auto const& endpoint : defaults)
    {
      os << ' ' << endpoint;
    }
    os << ")" << std::endl;
  }
  os << "  --logfile <path>                 " <<
    "log to file <path>" << std::endl;
  os << "  --logfile-rotation-depth <depth> " << 
    "sets logfile rotation depth (default: " <<
    cuti::file_backend_t::default_rotation_depth << ')' << std::endl;
  os << "  --logfile-size-limit <limit>     " <<
    "sets logfile size limit (default: none)" << std::endl;
  os << "  --loglevel <level>               " <<
    "sets loglevel (default: " << 
    cuti::loglevel_string(default_loglevel) << ')' << std::endl;
  os << "  --max-concurrent-requests <n>    " <<
    "sets max #concurrent requests" << std::endl;
  os << "                                     (default: " <<
    cuti::dispatcher_config_t::default_max_concurrent_requests() <<
    "; 0=unlimited) " << std::endl;
  os << "  --max-connections <n>            " <<
    "sets max #connections" << std::endl;
  os << "                                     (default: " <<
    cuti::dispatcher_config_t::default_max_connections() <<
    "; 0=unlimited) " << std::endl;
  os << "  --pidfile <path>                 " <<
    "create PID file <path> (default: none)" << std::endl;
  os << "  --preset <presets>               " <<
    "sets libx264 session presets (default: none)" << std::endl;
  os << "  --selector <type>                " <<
    "sets selector type (default: " <<
    cuti::dispatcher_config_t::default_selector_factory() << ")" << std::endl;
  os << "  --session-threads <n>            " <<
    "sets libx264 #encoding session threads" << std::endl;
  os << "                                     (default: " <<
    encoder_settings_t::default_session_threads() << "; 0=auto)" << std::endl;

  os << "  --session-lookahead-threads <n>  " <<
    "sets libx264 #encoding session lookahead threads" << std::endl;
  os << "                                     (default: " <<
    encoder_settings_t::default_session_lookahead_threads() << "; 0=auto)" <<
    std::endl;

  os << "  --session-sliced-threads         " <<
    "sets libx264 use of slice-based threading" << std::endl;

  os << "  --session-deterministic          " <<
    "sets libx264 use of deterministic optimizations" << std::endl;

  os << "  --session-cpu-independent        " <<
    "sets libx264 use of CPU-independent algorithms" << std::endl;

  os << "  --syslog                         " <<
    "log to system log as " << cuti::default_syslog_name(argv0_) <<
    std::endl;
  os << "  --syslog-name <name>             " <<
    "log to system log as <name>" << std::endl;
  os << "  --tune <tunings>                 " <<
    "sets libx264 session tunings (default: none)" << std::endl;
#ifndef _WIN32
  os << "  --umask <mask>                   " <<
    "set umask (default: no change)" << std::endl;
  os << "  --user <name>                    " <<
    "run as user <name>" << std::endl;
#endif
  os << std::endl;
  os << copyright_notice() << std::endl;
}

} // x264_es_utils
