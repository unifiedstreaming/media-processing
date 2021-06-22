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

#include "cmdline_reader.hpp"
#include "config_file_reader.hpp"
#include "dispatcher.hpp"
#include "endpoint.hpp"
#include "exception_builder.hpp"
#include "file_backend.hpp"
#include "listener.hpp"
#include "logger.hpp"
#include "option_walker.hpp"
#include "process.hpp"
#include "resolver.hpp"
#include "service.hpp"
#include "syslog_backend.hpp"
#include "tcp_acceptor.hpp"

#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace // anonymous
{

char const* copyright_notice()
{
  return
R"(Copyright (C) 2021 CodeShop B.V.

This program is free software. It comes with ABSOLUTELY NO WARRANTY
and is licensed to you under the terms of version 2 of the GNU General
Public License as published by the Free Software Foundation. Under
certain conditions, you may modify and/or redistribute this program;
see <http://www.gnu.org/licenses/> for details.)";
}

struct x264_client_t : cuti::client_t
{
  x264_client_t(cuti::logging_context_t& context,
                std::unique_ptr<cuti::tcp_connection_t> connection)
  : context_(context)
  , connection_(std::move(connection))
  {
    connection_->set_nonblocking();
    if(auto msg = context.message_at(cuti::loglevel_t::info))
    {
      *msg << "accepted client " << *connection_;
    }
  }

  cuti::cancellation_ticket_t call_when_readable(
    cuti::scheduler_t& scheduler, cuti::callback_t callback) override
  {
    return connection_->call_when_readable(scheduler, std::move(callback));
  }

  bool on_readable() override
  {
    return false;
  }
    
  ~x264_client_t() override
  {
    if(auto msg = context_.message_at(cuti::loglevel_t::info))
    {
      *msg << "disconnecting client " << *connection_;
    }
  }

private :
  cuti::logging_context_t& context_;
  std::unique_ptr<cuti::tcp_connection_t> connection_;
};

struct x264_listener_t : cuti::listener_t
{
  x264_listener_t(cuti::logging_context_t& context,
                  cuti::endpoint_t const& endpoint)
  : context_(context)
  , acceptor_(endpoint)
  {
    acceptor_.set_nonblocking();

    if(auto msg = context.message_at(cuti::loglevel_t::info))
    {
      *msg << "listening at endpoint " << endpoint;
    }
  }

  cuti::cancellation_ticket_t call_when_ready(
    cuti::scheduler_t& scheduler, cuti::callback_t callback) override
  {
    return acceptor_.call_when_ready(scheduler, std::move(callback));
  }

  std::unique_ptr<cuti::client_t> on_ready() override
  {
    std::unique_ptr<cuti::tcp_connection_t> accepted;
    std::unique_ptr<cuti::client_t> result;

    int error = acceptor_.accept(accepted);
    if(error != 0)
    {
      if(auto msg = context_.message_at(cuti::loglevel_t::error))
      {
        *msg << "listener " << acceptor_ << ": accept() failure: " <<
          cuti::system_error_string(error);
      }
    }
    else if(accepted == nullptr)
    {
      if(auto msg = context_.message_at(cuti::loglevel_t::warning))
      {
        *msg << "listener " << acceptor_ << ": accept() would block";
      }
    }
    else
    {
      result = std::make_unique<x264_client_t>(context_, std::move(accepted));
    }

    return result;
  }
    
private :
  cuti::logging_context_t& context_;
  cuti::tcp_acceptor_t acceptor_;
};
  
struct x264_encoding_service_t : cuti::service_t
{
  x264_encoding_service_t(cuti::logging_context_t& logging_context,
                          cuti::tcp_connection_t& control_connection,
                          cuti::selector_factory_t const& selector_factory,
                          std::vector<cuti::endpoint_t> const& endpoints)
  : dispatcher_(logging_context, control_connection, selector_factory)
  {
    for(auto const& endpoint : endpoints)
    {
      dispatcher_.add_listener(std::make_unique<x264_listener_t>(
        logging_context, endpoint));
    }
  }

  void run() override
  {
    dispatcher_.run();
  }
      
private :
  cuti::dispatcher_t dispatcher_;
};

std::vector<cuti::endpoint_t>
default_endpoints()
{
  return cuti::local_interfaces(11264);
}
  
struct x264_config_t : cuti::service_config_t
{
  x264_config_t(int argc, char const* const argv[])
  : argv0_((assert(argc > 0), argv[0]))
#ifndef _WIN32
  , daemon_(false)
#endif
  , directory_()
  , dry_run_(false)
  , endpoints_()
#ifndef _WIN32
  , group_()
#endif
  , logfile_()
  , loglevel_(default_loglevel)
  , pidfile_()
  , rotation_depth_(cuti::file_backend_t::default_rotation_depth)
  , selector_()
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

#ifndef _WIN32 // POSIX-only

  bool run_as_daemon() const override
  {
    return bool(daemon_);
  }

  cuti::group_id_t const* group_id() const override
  {
    return group_ ? &(*group_) : nullptr;
  }

  cuti::user_id_t const* user_id() const override
  {
    return user_ ? &(*user_) : nullptr;
  }

  cuti::umask_t const* umask() const override
  {
    return umask_ ? &(*umask_) : nullptr;
  }

#endif

  char const* directory() const override
  {
    return directory_.empty() ? nullptr : directory_.c_str();
  }

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
      result = std::make_unique<cuti::pidfile_t>(pidfile_);
    }

    return result;
  }

  std::unique_ptr<cuti::service_t>
  create_service(cuti::logging_context_t& context,
                 cuti::tcp_connection_t& control_connection) const override
  {
    context.level(loglevel_);

    auto endpoints = endpoints_;
    if(endpoints.empty())
    {
      endpoints = default_endpoints();
    }

    auto result = std::make_unique<x264_encoding_service_t>(
        context, control_connection, selector_, endpoints);
    if(dry_run_)
    {
      result.reset();
    }
    return result;
  }

private :
  void read_options(cuti::args_reader_t& reader)
  {
    read_options(reader, 0);
  }

  void read_options(cuti::args_reader_t& reader, int config_file_depth)
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
#ifndef _WIN32
        !walker.match("--group", group_) &&
#endif
        !walker.match("--loglevel", loglevel_) &&
        !walker.match("--pidfile", pidfile_) &&
        !walker.match("--rotation-depth", rotation_depth_) &&
        !walker.match("--selector", selector_) &&
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
          ": unknown option \'" << reader.current_argument() << "\'" <<
          std::endl;
        print_usage(builder);
        builder.explode();
      }
    }

    if(!reader.at_end())
    {
      cuti::exception_builder_t<std::runtime_error> builder;
      builder << reader.current_origin() <<
        ": unexpected argument \'" << reader.current_argument() << "\'" <<
        std::endl;
      print_usage(builder);
      builder.explode();
    }
  }

  void print_usage(std::ostream& os)
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
    os << "  --pidfile <path>         " <<
      "create PID file <path> (default: none)" << std::endl;
    os << "  --rotation-depth <depth> " << 
      "sets logfile rotation depth (default: " <<
      cuti::file_backend_t::default_rotation_depth << ')' << std::endl;
    os << "  --selector <type>        " <<
      "sets selector type (default: " <<
      cuti::selector_factory_t() << ")" << std::endl;
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

private :
  static constexpr cuti::loglevel_t default_loglevel = 
    cuti::loglevel_t::warning;

  std::string argv0_;

#ifndef _WIN32
  cuti::flag_t daemon_;
#endif
  std::string directory_;
  cuti::flag_t dry_run_;
  std::vector<cuti::endpoint_t> endpoints_;
#ifndef _WIN32
  std::optional<cuti::group_id_t> group_;
#endif
  cuti::absolute_path_t logfile_;
  cuti::loglevel_t loglevel_;
  cuti::absolute_path_t pidfile_;
  unsigned int rotation_depth_;
  cuti::selector_factory_t selector_;
  unsigned int size_limit_;
  cuti::flag_t syslog_;  
  std::string syslog_name_;
#ifndef _WIN32
  std::optional<cuti::umask_t> umask_;
  std::optional<cuti::user_id_t> user_;
#endif
};

struct x264_config_reader_t : cuti::service_config_reader_t
{
  std::unique_ptr<cuti::service_config_t>
  read_config(int argc, char const* const argv[]) const override
  {
    return std::make_unique<x264_config_t>(argc, argv);
  }
};
    
void throwing_main(int argc, char const* const argv[])
{
  x264_config_reader_t config_reader;
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
    std::cerr << argv[0] << ": " << ex.what() << std::endl;
  }

  return result;
}
