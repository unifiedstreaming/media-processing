/*
 * Copyright (C) 2022 CodeShop B.V.
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

#include "event_pipe.hpp"

#include "io_utils.hpp"
#include "tcp_connection.hpp"
#include "scheduler.hpp"
#include "scoped_guard.hpp"
#include "system_error.hpp"

#include <cassert>
#include <limits>
#include <memory>
#include <tuple>
#include <utility>

#if !defined(_WIN32)
#include <fcntl.h>
#include <unistd.h>
#endif

#if defined(__linux__) || defined(__FreeBSD__)
#define CUTI_HAS_PIPE2 1
#else
#undef  CUTI_HAS_PIPE2
#endif

namespace cuti
{

#if defined(_WIN32)

namespace // anonymous
{

struct event_pipe_reader_impl_t : event_pipe_reader_t
{
  explicit event_pipe_reader_impl_t(std::unique_ptr<tcp_connection_t> conn)
  : conn_(std::move(conn))
  { }

  void set_blocking() override
  {
    conn_->set_blocking();
  }

  void set_nonblocking() override
  {
    conn_->set_nonblocking();
  }

  std::optional<int> read() override
  {
    char buf[1];
    char* next;

    int r = conn_->read(buf, buf + 1, next);
    if(r != 0)
    {
      throw system_exception_t("event pipe read error", r);
    }

    std::optional<int> result = std::nullopt;

    if(next == buf)
    {
      result.emplace(eof);
    }
    else if(next == buf + 1)
    {
      result.emplace(std::char_traits<char>::to_int_type(*buf));
    }
    else
    {
      assert(next == nullptr);
    }
    
    return result;  
  }

  cancellation_ticket_t call_when_readable(
    scheduler_t& scheduler, callback_t callback) const override
  {
    return conn_->call_when_readable(scheduler, std::move(callback));
  }

private :
  std::unique_ptr<tcp_connection_t> conn_;
};

struct event_pipe_writer_impl_t : event_pipe_writer_t
{
  explicit event_pipe_writer_impl_t(std::unique_ptr<tcp_connection_t> conn)
  : conn_(std::move(conn))
  { }

  void set_blocking() override
  {
    conn_->set_blocking();
  }

  void set_nonblocking() override
  {
    conn_->set_nonblocking();
  }

  bool write(unsigned char event) override
  {
    char buf[1];
    buf[0] = static_cast<char>(event);
    char const* next;
    
    int r = conn_->write(buf, buf + 1, next);
    if(r != 0)
    {
      throw system_exception_t("event pipe write error", r);
    }

    if(next == nullptr)
    {
      return false;
    }

    assert(next == buf + 1);
    return true;
  }

  cancellation_ticket_t call_when_writable(
    scheduler_t& scheduler, callback_t callback) const override
  {
    return conn_->call_when_writable(scheduler, std::move(callback));
  }

private :
  std::unique_ptr<tcp_connection_t> conn_;
};

} // anonymous

std::pair<std::unique_ptr<event_pipe_reader_t>,
          std::unique_ptr<event_pipe_writer_t>>
make_event_pipe()
{
  std::unique_ptr<tcp_connection_t> tcp_read_end;
  std::unique_ptr<tcp_connection_t> tcp_write_end;
  
  std::tie(tcp_read_end, tcp_write_end) = make_connected_pair();

  auto read_end = std::make_unique<event_pipe_reader_impl_t>(
    std::move(tcp_read_end));
  auto write_end = std::make_unique<event_pipe_writer_impl_t>(
    std::move(tcp_write_end));
  
  return std::make_pair(std::move(read_end), std::move(write_end));
}

#else // POSIX

namespace // anonymous
{

struct event_pipe_reader_impl_t : event_pipe_reader_t
{
  explicit event_pipe_reader_impl_t(int fd)
  : fd_(fd)
  { }

  void set_blocking() override
  {
    cuti::set_nonblocking(fd_, false);
  }

  void set_nonblocking() override
  {
    cuti::set_nonblocking(fd_, true);
  }

  std::optional<int> read() override
  {
    char buf[1];
    auto r = ::read(fd_, buf, 1);

    std::optional<int> result = std::nullopt;

    switch(r)
    {
    case -1 :
      {
        int cause = last_system_error();
        if(!is_wouldblock(cause))
        {
          throw system_exception_t("event pipe read error", cause);
        }
      }
      break;
    case 0 :
      result.emplace(eof);
      break;
    default :
      assert(r == 1);
      result.emplace(std::char_traits<char>::to_int_type(*buf));
      break;
    }

    return result;
  }

  cancellation_ticket_t call_when_readable(
    scheduler_t& scheduler, callback_t callback) const override
  {
    return scheduler.call_when_readable(fd_, std::move(callback));
  }

  ~event_pipe_reader_impl_t() override
  {
    ::close(fd_);
  }

private :
  int fd_;
};

struct event_pipe_writer_impl_t : event_pipe_writer_t
{
  explicit event_pipe_writer_impl_t(int fd)
  : fd_(fd)
  { }

  void set_blocking() override
  {
    cuti::set_nonblocking(fd_, false);
  }

  void set_nonblocking() override
  {
    cuti::set_nonblocking(fd_, true);
  }

  bool write(unsigned char event) override
  {
    char buf[1];
    buf[0] = static_cast<char>(event);

    auto r = ::write(fd_, buf, 1);
    if(r == -1)
    {
      int cause = last_system_error();
      if(!is_wouldblock(cause))
      {
        throw system_exception_t("event pipe write error", cause);
      }
      return false;
    }

    assert(r == 1);
    return true;
  }

  cancellation_ticket_t call_when_writable(
    scheduler_t& scheduler, callback_t callback) const override
  {
    return scheduler.call_when_writable(fd_, std::move(callback));
  }

  ~event_pipe_writer_impl_t() override
  {
    ::close(fd_);
  }

private :
  int fd_;
};

} // anonymous

std::pair<std::unique_ptr<event_pipe_reader_t>,
          std::unique_ptr<event_pipe_writer_t>>
make_event_pipe()
{
  int fds[2];

#if defined(CUTI_HAS_PIPE2)
  int r = ::pipe2(fds, O_CLOEXEC);
#else
  int r = ::pipe(fds);
#endif

  if(r == -1)
  {
    int cause = last_system_error();
    throw system_exception_t("can\'t create event pipe", cause);
  }

  auto fds0_guard = make_scoped_guard([&] { ::close(fds[0]); });
  auto fds1_guard = make_scoped_guard([&] { ::close(fds[1]); });
  
#if !defined(CUTI_HAS_PIPE2)
  set_cloexec(fds[0], true);
  set_cloexec(fds[1], true);
#endif

  auto read_end = std::make_unique<event_pipe_reader_impl_t>(fds[0]);
  fds0_guard.dismiss();

  auto write_end = std::make_unique<event_pipe_writer_impl_t>(fds[1]);
  fds1_guard.dismiss();
  
  return std::make_pair(std::move(read_end), std::move(write_end));
}
  
#endif // POSIX

} // cuti
