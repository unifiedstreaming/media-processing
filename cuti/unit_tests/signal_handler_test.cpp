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

#include "signal_handler.hpp"

#include "chrono_types.hpp"
#include "default_scheduler.hpp"
#include "option_walker.hpp"
#include "selector_factory.hpp"
#include "tcp_connection.hpp"

#include <iostream>
#include <thread>

// Enable assert()
#undef NDEBUG
#include <cassert>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#endif

namespace // anonymous
{

using namespace cuti;

#ifdef _WIN32

/*
 * On Windows, it is close to impossible to programmatically send a
 * CTRL-C to the current process without affecting other processes in
 * the console session.  Not even trying here.
 */
void automated_tests()
{ }

#else // POSIX

void trap(int sig1, int sig2)
{
  std::unique_ptr<tcp_connection_t> sender;
  std::unique_ptr<tcp_connection_t> receiver;
  std::tie(sender, receiver) = make_connected_pair();
  sender->set_nonblocking();

  auto send_sig1 = [&]
  {
    char buf[1];
    buf[0] = static_cast<char>(sig1);
    char const* next;
    sender->write(buf, buf + 1, next);
  };
  signal_handler_t handler1(sig1, send_sig1);
    
  auto send_sig2 = [&]
  {
    char buf[1];
    buf[0] = static_cast<char>(sig2);
    char const* next;
    sender->write(buf, buf + 1, next);
  };
  signal_handler_t handler2(sig2, send_sig2);

  kill(getpid(), sig1);
  kill(getpid(), sig2);

  bool got_sig1 = false;
  bool got_sig2 = false;
  for(int i = 0; i != 2; ++i)
  {
    char buf[1];
    char* next;
    receiver->read(buf, buf + 1, next);
    assert(next == buf + 1);
    if(buf[0] == sig1)
    {
      got_sig1 = true;
    }
    else if(buf[0] == sig2)
    {
      got_sig2 = true;
    }
  }

  assert(got_sig1);
  assert(got_sig2);
}

void ignore(int sig1, int sig2)
{
  signal_handler_t handler1(sig1, nullptr);
  signal_handler_t handler2(sig2, nullptr);

  kill(getpid(), sig1);
  kill(getpid(), sig2);
}

void nested(int sig1, int sig2)
{
  signal_handler_t handler1(sig1, []{ });
  signal_handler_t handler2(sig2, []{ });
  trap(sig1, sig2);
  ignore(sig1, sig2);
}
  

void automated_tests()
{
  trap(SIGINT, SIGTERM);
  ignore(SIGINT, SIGTERM);
  nested(SIGINT, SIGTERM);
}
  
#endif // POSIX

int interactive_trap()
{
  std::unique_ptr<tcp_connection_t> sender;
  std::unique_ptr<tcp_connection_t> receiver;
  std::tie(sender, receiver) = make_connected_pair();
  sender->set_nonblocking();

  auto send_sigint = [&]
  {
    char buf[1];
    buf[0] = static_cast<char>(SIGINT);
    char const* next;
    sender->write(buf, buf + 1, next);
  };
  signal_handler_t handler(SIGINT, send_sigint);
  std::cout << "Trapping SIGINT: 10 seconds to hit ^C..." << std::endl;

  default_scheduler_t scheduler;
  bool timeout = false;
  scheduler.call_alarm(seconds_t(10), [&] { timeout = true; });
  bool readable = false;
  receiver->call_when_readable(scheduler, [&] { readable = true; });

  while(!timeout && !readable)
  {
    callback_t callback = scheduler.wait();
    assert(callback != nullptr);
    callback();
  }

  if(timeout)
  {
    std::cout << "Trapping SIGINT: timeout; failed" << std::endl;
    return 1;
  }

  char buf[1];
  char *next;
  receiver->read(buf, buf + 1, next);
  assert(next == buf + 1);
  assert(buf[0] == SIGINT);

  std::cout << "Trapping SIGINT: got SIGINT; succeeded" << std::endl;

  return 0;
}
  
int interactive_trap_then_ignore()
{
  signal_handler_t handler(SIGINT, nullptr);

  int r = interactive_trap();
  if(r != 0)
  {
    return r;
  }

  std::cout << "Ignoring SIGINT: 10 seconds to hit ^C..." << std::endl;

  default_scheduler_t scheduler;

  bool timeout = false;
  scheduler.call_alarm(seconds_t(10), [&] { timeout = true; });
  while(!timeout)
  {
    callback_t callback = scheduler.wait();
    assert(callback != nullptr);
    callback();
  }

  std::cout << "Ignoring SIGINT: timeout; succeeded" << std::endl;
  return 0;
}

int manual_tests()
{
  int errors = 0;
  errors += interactive_trap_then_ignore();
  return errors ? 1 : 0;
}

void usage(char const* argv0)
{
  std::cerr << "usage: " << argv0 << " [<option>...]" << std::endl;
  std::cerr << "options are:" << std::endl;
  std::cerr << "  --manual  run manual tests" << std::endl;
  std::cerr << "      (no automated tests on Windows)" << std::endl;
}


} // anonymous

int main(int argc, char *argv[])
{
  cuti::flag_t manual = false;
  cuti::option_walker_t walker(argc, argv);
  while(!walker.done())
  {
    if(!walker.match("--manual", manual))
    {
      break;
    }
  }
  if(!walker.done() || walker.next_index() != argc)
  {
    usage(argv[0]);
    return 1;
  }
  
  int result = 0;
  if(manual)
  {
    result = manual_tests();
  }
  else
  {
    automated_tests();
  }
  return result;
}

