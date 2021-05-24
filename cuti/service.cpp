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

#include "service.hpp"
#include "signal_handler.hpp"
#include "streambuf_backend.hpp"
#include "syslog_backend.hpp"
#include "system_error.hpp"

#include <cassert>
#include <exception>
#include <iostream>
#include <limits>
#include <memory>
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

void send_signal(tcp_connection_t& control_sender, int sig)
{
  char buf[1];
  buf[0] = static_cast<char>(sig);
  char const* next;
  control_sender.write(buf, buf + 1, next);
}
                 
#ifdef _WIN32

struct service_main_args_t
{
  int argc_;
  char const* const* argv_;
  service_config_reader_t const* config_reader_;
  tcp_connection_t* control_sender_;
  tcp_connection_t* control_receiver_;
};

/*
 * Global variable alert: saved here so service_main can pick these
 * up.
 */
service_main_args_t service_main_args;

void control_handler(DWORD control)
{
  assert(service_main_args.control_sender_ != nullptr);

  if(control == SERVICE_CONTROL_STOP)
  {
    send_signal(*service_main_args.control_sender_, SIGTERM);
  }
}

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
  
  void report_running()
  {
    SERVICE_STATUS status;
    std::memset(&status, '\0', sizeof status);
    status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    status.dwCurrentState = SERVICE_RUNNING;
    status.dwControlsAccepted = SERVICE_ACCEPT_STOP;

    if(!SetServiceStatus(handle_, &status))
    {
      int cause = last_system_error();
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
  static constexpr DWORD wait_hint = 30000;

  int exit_code_;
  SERVICE_STATUS_HANDLE handle_;
};

#else // POSIX

struct pipe_t
{
  pipe_t()
  {
    if(::pipe(fds_) == -1)
    {
      int cause = last_system_error();
      throw system_exception_t("can't create pipe", cause);
    }
  }

  pipe_t(pipe_t const&) = delete;
  pipe_t operator=(pipe_t const&) = delete;

  void write(char const* first, char const* last, char const*& next)
  {
    assert(fds_[1] != -1);
    assert(first < last);

    int count = std::numeric_limits<int>::max();
    if(count > last - first)
    {
      count = static_cast<int>(last - first);
    }

    auto r = ::write(fds_[1], first, count);
    if(r == -1)
    {
      int cause = last_system_error();
      throw system_exception_t("can't write to pipe", cause);
    }

    next = first + r;
  }

  void close_write_end()
  {
    assert(fds_[1] != -1);
    ::close(fds_[1]);
    fds_[1] = -1;
  }

  void read(char* first, char const* last, char*& next)
  {
    assert(fds_[0] != -1);
    assert(first < last);
    
    int count = std::numeric_limits<int>::max();
    if(count > last - first)
    {
      count = static_cast<int>(last - first);
    }

    auto r = ::read(fds_[0], first, count);
    if(r == -1)
    {
      int cause = last_system_error();
      throw system_exception_t("can't read from pipe", cause);
    }

    next = first + r;
  }

  void close_read_end()
  {
    assert(fds_[0] != -1);
    ::close(fds_[0]);
    fds_[0] = -1;
  }

  ~pipe_t()
  {
    if(fds_[1] != -1)
    {
      ::close(fds_[1]);
    }
    if(fds_[0] != -1)
    {
      ::close(fds_[0]);
    }
  }

private :
  int fds_[2];
};

struct status_reporter_t
{
  explicit status_reporter_t(pipe_t& pipe)
  : pipe_(pipe)
  { }

  status_reporter_t(status_reporter_t const&) = delete;
  status_reporter_t& operator=(status_reporter_t const&) = delete;
  
  void report_running()
  {
    char buf[1];
    buf[0] = '\0';
    char const* next;
    pipe_.write(buf, buf + 1, next);
    pipe_.close_write_end();
  }

  void report_stopping(int exit_code) noexcept
  { }

  ~status_reporter_t()
  { }

private :
  pipe_t& pipe_;
};

#endif // POSIX

struct running_state_t
{
  explicit running_state_t(status_reporter_t& reporter)
  : exit_code_(1)
  , reporter_(reporter)
  {
    reporter_.report_running();
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

void create_and_run_service(logger_t& logger,
                            service_config_t const& config,
                            tcp_connection_t& control_connection)
{
  logging_context_t context(logger, default_loglevel);
  if(auto service = config.create_service(context, control_connection))
  {
    service->run();
  }
}

void create_and_run_service(logger_t& logger,
                            service_config_t const& config,
                            tcp_connection_t& control_connection,
                            status_reporter_t& status_reporter)
{
  running_state_t running_state(status_reporter);
  create_and_run_service(logger, config, control_connection);
  running_state.set_success();
}  
  
#ifdef _WIN32

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
  
  assert(service_main_args.control_receiver_ != nullptr);
  tcp_connection_t& control_connection =
    *service_main_args.control_receiver_;

  /*
   * To be able to report configuration errors, we enable a
   * default-configured logger before parsing the command line.
   */
  logger_t logger(argv[0]);
  logger.set_backend(std::make_unique<syslog_backend_t>(
    default_service_name(argv[0]).c_str()));

  logging_context_t context(logger, default_loglevel);
  try
  {
    auto config = config_reader.read_config(argc, argv);
    assert(config != nullptr);

    if(auto backend = config->create_logging_backend())
    {
      logger.set_backend(std::move(backend));
    }

    status_reporter_t status_reporter;
    create_and_run_service(
      logger, *config, control_connection, status_reporter);
  }
  catch(std::exception const& ex)
  {
    if(auto msg = context.message_at(loglevel_t::error))
    {
      *msg << "exception: " << ex.what();
    }
  }
}

void run_from_console(service_config_reader_t const& config_reader,
                      int argc, char const* const argv[],
                      tcp_connection_t& control_sender,
                      tcp_connection_t& control_receiver)
{
  auto config = config_reader.read_config(argc, argv);
  assert(config != nullptr);

  logger_t logger(argv[0]);
  if(auto backend = config->create_logging_backend())
  {
    logger.set_backend(std::move(backend));
  }
  else
  {
    logger.set_backend(std::make_unique<streambuf_backend_t>(std::cerr));
  }

  logging_context_t context(logger, default_loglevel);
  try
  {
    auto on_sigint = [&] { send_signal(control_sender, SIGINT); };
    signal_handler_t sigint_handler(SIGINT, on_sigint);

    create_and_run_service(logger, *config, control_receiver);
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
    
#else // POSIX

void run_as_daemon(logger_t& logger, service_config_t const& config)
{
  pipe_t to_grandparent;

  auto child = ::fork();
  if(child == -1)
  {
    int cause = last_system_error();
    throw system_exception_t("fork() failure", cause);
  }

  if(child == 0)
  {
    // in child
    to_grandparent.close_read_end();

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
      
      // redirect standard fds
      int dev_null = ::open("/dev/null", O_RDWR);
      if(dev_null == -1)
      {
        int cause = last_system_error();
        throw system_exception_t("can't open /dev/null", cause);
      }
      
      assert(dev_null > 2);
      for(int fd = 0; fd <= 2; ++fd)
      {
        int r = ::dup2(dev_null, fd);
        if(r == -1)
        {
          int cause = last_system_error();
          throw system_exception_t("dup2() failure", cause);
        }
      }
      ::close(dev_null);

      std::unique_ptr<tcp_connection_t> control_sender;
      std::unique_ptr<tcp_connection_t> control_receiver;
      std::tie(control_sender, control_receiver) = make_connected_pair();
      control_sender->set_nonblocking();

      auto on_sigterm = [&] { send_signal(*control_sender, SIGTERM); };
      signal_handler_t sigterm_handler(SIGTERM, on_sigterm);
  
      status_reporter_t reporter(to_grandparent);
      create_and_run_service(logger, config, *control_receiver, reporter);
    }

    return;
  }

  // in grandparent
  to_grandparent.close_write_end();
  
  // await child
  int status;
  auto wait_r = ::waitpid(child, &status, 0);
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
    wait_r = ::waitpid(child, &status, 0);
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
        
  // await grandchild's confirmation
  char buf[1];
  char* next;
  to_grandparent.read(buf, buf + 1, next);
  if(next != buf + 1)
  {
    throw system_exception_t(
      "service failed to initialize; please check the system log "
      "or the service's log files");
  }
}

void run_in_foreground(logger_t& logger, service_config_t const& config)
{
  std::unique_ptr<tcp_connection_t> control_sender;
  std::unique_ptr<tcp_connection_t> control_receiver;
  std::tie(control_sender, control_receiver) = make_connected_pair();
  control_sender->set_nonblocking();

  auto on_sigint = [&] { send_signal(*control_sender, SIGINT); };
  signal_handler_t sigint_handler(SIGINT, on_sigint);
  
  create_and_run_service(logger, config, *control_receiver);
}
  
#endif // POSIX

} // anonymous

#ifdef _WIN32

void run_service(service_config_reader_t const& config_reader,
                 int argc, char const* const argv[])
{
  std::unique_ptr<tcp_connection_t> control_sender;
  std::unique_ptr<tcp_connection_t> control_receiver;
  std::tie(control_sender, control_receiver) = make_connected_pair();
  control_sender->set_nonblocking();

  // Initialize args for service_main
  service_main_args.argc_ = argc;
  service_main_args.argv_ = argv;
  service_main_args.config_reader_ = &config_reader;
  service_main_args.control_sender_ = control_sender.get();
  service_main_args.control_receiver_ = control_receiver.get();

  static constexpr SERVICE_TABLE_ENTRY service_table[] = {
    { "", service_main },
    { nullptr, nullptr }
  };

  // Try to run as a service
  if(StartServiceCtrlDispatcher(service_table))
  {
    // service_main() does all the work and returns here; done
  }
  else
  {
    int cause = last_system_error();
    if(cause != ERROR_FAILED_SERVICE_CONTROLLER_CONNECT)
    {
      throw system_exception_t("StartServiceCtrlDispatcher() failure", cause);
    }
    run_from_console(config_reader, argc, argv,
      *control_sender, *control_receiver);
  }
}
  
#else // POSIX

void run_service(service_config_reader_t const& config_reader,
                 int argc, char const* const argv[])
{
  auto config = config_reader.read_config(argc, argv);
  assert(config != nullptr);

  logger_t logger(argv[0]);
  if(auto backend = config->create_logging_backend())
  {
    logger.set_backend(std::move(backend));
  }
  else if(config->run_as_daemon())
  {
    logger.set_backend(std::make_unique<syslog_backend_t>(
      default_service_name(argv[0]).c_str()));
  }
  else
  {
    logger.set_backend(std::make_unique<streambuf_backend_t>(std::cerr));
  }

  logging_context_t context(logger, default_loglevel);
  try
  {
    if(config->run_as_daemon())
    {
      run_as_daemon(logger, *config);
    }
    else
    {
      run_in_foreground(logger, *config);
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

} // cuti
