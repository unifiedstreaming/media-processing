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

#include "service.hpp"

#include "event_pipe.hpp"
#include "fs_utils.hpp"
#include "scoped_guard.hpp"
#include "signal_handler.hpp"
#include "streambuf_backend.hpp"
#include "syslog_backend.hpp"
#include "system_error.hpp"

#include <cassert>
#include <exception>
#include <iostream>
#include <memory>
#include <tuple>
#include <utility>

#ifdef _WIN32

#include <cstring>
#include <vector>

#include <windows.h>

#else

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#endif

namespace cuti
{

namespace // anonymous
{

constexpr auto default_loglevel = loglevel_t::warning;

void run_attended(service_config_t const& config, char const* argv0)
{
#ifndef _WIN32
  if(auto group_id = config.group_id())
  {
    group_id->apply();
  }

  if(auto user_id = config.user_id())
  {
    user_id->apply();
  }

  if(auto umask = config.umask())
  {
    umask->apply();
  }
#endif

  auto logging_backend = config.create_logging_backend();
  if(!logging_backend)
  {
    logging_backend = std::make_unique<streambuf_backend_t>(std::cerr);
  }

  logger_t logger(std::move(logging_backend));

  auto pidfile = config.create_pidfile();

  if(auto dir = config.directory())
  {
    cuti::change_directory(dir);
  }

  logging_context_t context(logger, default_loglevel);
  if(auto service = config.create_service(context))
  {
    signal_handler_t handler(SIGINT, [&] { service->stop(SIGINT); });
    service->run();
  }
}

} // anonymous

#ifdef _WIN32

namespace // anonymous
{

struct service_main_args_t
{
  int argc_;
  char const* const* argv_;
  service_config_reader_t const* config_reader_;
};

/*
 * Global variable alert: saved here so service_main can pick these
 * up.
 */
service_main_args_t service_main_args;

struct status_reporter_t
{
  status_reporter_t()
  : exit_code_(1)
  , handle_(RegisterServiceCtrlHandler("", control_handler))
  {
    if(handle_ == 0)
    {
      int cause = last_system_error();
      throw system_exception_t("RegisterServiceCtrlHandler() failure", cause);
    }

    SERVICE_STATUS status;
    std::memset(&status, '\0', sizeof status);
    status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    status.dwCurrentState = SERVICE_START_PENDING;
    status.dwWaitHint = wait_hint;

    if(!SetServiceStatus(handle_, &status))
    {
      int cause = last_system_error();
      throw system_exception_t("SetServiceStatus() failure", cause);
    }
  }

  status_reporter_t(status_reporter_t const&) = delete;
  status_reporter_t& operator=(status_reporter_t const&) = delete;
  
  void report_running(service_t *service)
  {
    assert(current_service_ == nullptr);
    current_service_ = service;

    SERVICE_STATUS status;
    std::memset(&status, '\0', sizeof status);
    status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    status.dwCurrentState = SERVICE_RUNNING;
    status.dwControlsAccepted =
      SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;

    if(!SetServiceStatus(handle_, &status))
    {
      int cause = last_system_error();
      current_service_ = nullptr;
      throw system_exception_t("SetServiceStatus() failure", cause);
    }
  }

  void report_stopping(int exit_code) noexcept
  {
    SERVICE_STATUS status;
    std::memset(&status, '\0', sizeof status);
    status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    status.dwCurrentState = SERVICE_STOP_PENDING;
    status.dwWaitHint = wait_hint;

    SetServiceStatus(handle_, &status);

    current_service_ = nullptr;
    exit_code_ = exit_code;
  }

  ~status_reporter_t()
  {
    SERVICE_STATUS status;
    std::memset(&status, '\0', sizeof status);
    status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    status.dwCurrentState = SERVICE_STOPPED;
    status.dwWin32ExitCode = exit_code_;

    SetServiceStatus(handle_, &status);
  }

private :
  static void control_handler(DWORD control)
  {
    switch(control)
    {
    case SERVICE_CONTROL_STOP :
    case SERVICE_CONTROL_SHUTDOWN :
      if(current_service_ != nullptr)
      {
        current_service_->stop(SIGTERM);
      }
      break;
    default :
      break;
    }
  }

private :
  static constexpr DWORD wait_hint = 30000;
  static service_t* current_service_;

  int exit_code_;
  SERVICE_STATUS_HANDLE handle_;
};

service_t* status_reporter_t::current_service_ = nullptr;

struct running_state_t
{
  running_state_t(status_reporter_t& reporter, service_t* service)
  : exit_code_(1)
  , reporter_(reporter)
  {
    reporter_.report_running(service);
  }

  running_state_t(running_state_t const&) = delete;
  running_state_t& operator=(running_state_t const&) = delete;

  void set_success() noexcept
  {
    exit_code_ = 0;
  }

  ~running_state_t()
  {
    reporter_.report_stopping(exit_code_);
  }
    
private :
  int exit_code_;
  status_reporter_t& reporter_;
};

void service_main(DWORD dwNumServiceArgs, LPSTR* lpServiceArgVectors)
{
  std::vector<char const*> args;

  // Pick up arguments from service_main_args
  assert(service_main_args.argc_ > 0);
  assert(service_main_args.argv_ != nullptr);
  for(int i = 0; i < service_main_args.argc_; ++i)
  {
    assert(service_main_args.argv_[i] != nullptr);
    args.push_back(service_main_args.argv_[i]);
  }

  // Pick up additional arguments from lpServiceArgVectors
  for(DWORD i = 1; i < dwNumServiceArgs; ++i)
  {
    assert(lpServiceArgVectors != nullptr);
    assert(lpServiceArgVectors[i] != nullptr);
    args.push_back(lpServiceArgVectors[i]);
  }

  int argc = static_cast<int>(args.size());
  char const* const* argv = args.data();
  
  assert(service_main_args.config_reader_ != nullptr);
  service_config_reader_t const& config_reader =
    *service_main_args.config_reader_;
  
  /*
   * To be able to report configuration errors, we enable a
   * default-configured logger before parsing the command line.
   */
  logger_t logger(std::make_unique<syslog_backend_t>(
    default_syslog_name(argv[0]).c_str()));

  logging_context_t context(logger, default_loglevel);
  try
  {
    status_reporter_t status_reporter;

    auto config = config_reader.read_config(argc, argv);
    assert(config != nullptr);

    // Set up final logger ASAP
    if(auto backend = config->create_logging_backend())
    {
      logger.set_backend(std::move(backend));
    }

    auto pidfile = config->create_pidfile();

    if(auto dir = config->directory())
    {
      cuti::change_directory(dir);
    }

    auto service = config->create_service(context);

    running_state_t running_state(status_reporter, service.get());
    if(service !=  nullptr)
    {
      service->run();
    }
    running_state.set_success();
  }
  catch(std::exception const& ex)
  {
    if(auto msg = context.message_at(loglevel_t::error))
    {
      *msg << "exception: " << ex.what();
    }
  }
}

} // anonymous

void run_service(service_config_reader_t const& config_reader,
                 int argc, char const* const argv[])
{
  assert(argc > 0);

  // Initialize args for service_main
  service_main_args.argc_ = argc;
  service_main_args.argv_ = argv;
  service_main_args.config_reader_ = &config_reader;

  static constexpr SERVICE_TABLE_ENTRY service_table[] = {
    { "", service_main },
    { nullptr, nullptr }
  };

  // Try to run as a service
  if(StartServiceCtrlDispatcher(service_table))
  {
    // StartServiceCtrlDispatcher() runs service_main(); done
  }
  else
  {
    int cause = last_system_error();
    if(cause != ERROR_FAILED_SERVICE_CONTROLLER_CONNECT)
    {
      throw system_exception_t("StartServiceCtrlDispatcher() failure", cause);
    }

    auto config = config_reader.read_config(argc, argv);
    assert(config != nullptr);

    run_attended(*config, argv[0]);
  }
}
  
#else // POSIX

namespace // anonymous
{

struct confirmation_pipe_t
{
  confirmation_pipe_t()
  : reader_()
  , writer_()
  {
    std::tie(reader_, writer_) = make_event_pipe();
  }

  confirmation_pipe_t(confirmation_pipe_t const&) = delete;
  confirmation_pipe_t operator=(confirmation_pipe_t const&) = delete;

  void confirm()
  {
    reader_.reset();

    bool wr = writer_->write('\0');
    static_cast<void>(wr);
    assert(wr);

    writer_.reset();
  }

  void await()
  {
    writer_.reset();

    std::optional<int> rr = reader_->read();
    assert(rr != std::nullopt);

    if(*rr == eof)
    {
      throw system_exception_t("service failed to initialize");
    }

    reader_.reset();
  }
    
private :
  std::unique_ptr<event_pipe_reader_t> reader_;
  std::unique_ptr<event_pipe_writer_t> writer_;
};

void redirect_standard_fds()
{
  int dev_null = ::open("/dev/null", O_RDWR);
  if(dev_null == -1)
  {
    int cause = last_system_error();
    throw system_exception_t("can't open /dev/null", cause);
  }
  assert(dev_null > 2);
  auto dev_null_guard = make_scoped_guard([&] { ::close(dev_null); });
  
  for(int fd = 0; fd <= 2; ++fd)
  {
    int r = ::dup2(dev_null, fd);
    if(r == -1)
    {
      int cause = last_system_error();
      throw system_exception_t("dup2() failure", cause);
    }
  }
}

void await_child(pid_t pid)
{
  int status;
  auto wait_r = ::waitpid(pid, &status, 0);
  while(wait_r == -1 || WIFSTOPPED(status))
  {
    if(wait_r == -1)
    {
      int cause = last_system_error();
      if(cause != EINTR)
      {
        throw system_exception_t("waitpid() failure", cause);
      }
    }
    wait_r = ::waitpid(pid, &status, 0);
  }
        
  // check child's exit status
  if(WIFSIGNALED(status))
  {
    system_exception_builder_t builder;
    builder << "run_service(): child killed by signal " << WTERMSIG(status);
    builder.explode();
  }

  assert(WIFEXITED(status));
  int exit_code = WEXITSTATUS(status);
  if(exit_code != 0)
  {
    system_exception_builder_t builder;
    builder << "run_service(): child reported exit code " << exit_code;
    builder.explode();
  }
}

void run_as_daemon(service_config_t const& config, char const* argv0)
{
  confirmation_pipe_t confirmation_pipe;

  auto child = ::fork();
  if(child == -1)
  {
    int cause = last_system_error();
    throw system_exception_t("fork() failure", cause);
  }

  if(child == 0)
  {
    // in child
    if(::setsid() == -1)
    {
      int cause = last_system_error();
      throw system_exception_t("setsid() failure", cause);
    }

    auto grandchild = ::fork();
    if(grandchild == -1)
    {
      int cause = last_system_error();
      throw system_exception_t("fork() failure", cause);
    }

    if(grandchild == 0)
    {
      // in grandchild
      if(auto group_id = config.group_id())
      {
        group_id->apply();
      }

      if(auto user_id = config.user_id())
      {
        user_id->apply();
      }

      if(auto umask = config.umask())
      {
        umask->apply();
      }
      
      auto logging_backend = config.create_logging_backend();
      if(!logging_backend)
      {
        logging_backend = std::make_unique<syslog_backend_t>(
          default_syslog_name(argv0));
      }

      logger_t logger(std::move(logging_backend));

      auto pidfile = config.create_pidfile();

      if(auto dir = config.directory())
      {
        cuti::change_directory(dir);
      }

      logging_context_t context(logger, default_loglevel);
      auto service = config.create_service(context);

      auto request_stop = [&]
      {
        if(service != nullptr)
        {
          service->stop(SIGTERM);
        }
      };
      signal_handler_t handler(SIGTERM, request_stop);

      redirect_standard_fds();

      try
      {
        confirmation_pipe.confirm();
        if(service != nullptr)
        {
          service->run();
        }
      }
      catch(std::exception const& ex)
      {
        if(auto msg = context.message_at(loglevel_t::error))
        {
          *msg << "exception: " << ex.what();
        }
        throw;
      }
    }
  }
  else
  {
    // in parent
    await_child(child);
    confirmation_pipe.await();
  }
}

void run_in_foreground(service_config_t const& config, char const* argv0)
{
  run_attended(config, argv0);
}
  
} // anonymous

void run_service(service_config_reader_t const& config_reader,
                 int argc, char const* const argv[])
{
  assert(argc > 0);

  auto config = config_reader.read_config(argc, argv);
  assert(config != nullptr);

  if(config->run_as_daemon())
  {
    run_as_daemon(*config, argv[0]);
  }
  else
  {
    run_in_foreground(*config, argv[0]);
  }
}
  
#endif // POSIX

service_t::service_t()
{ }

service_t::~service_t()
{ }

service_config_t::service_config_t()
{ }

service_config_t::~service_config_t()
{ }

service_config_reader_t::service_config_reader_t()
{ }

service_config_reader_t::~service_config_reader_t()
{ }

} // cuti
