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

#include "process.hpp"

#include "fs_utils.hpp"
#include "scoped_guard.hpp"
#include "system_error.hpp"

#include <cassert>
#include <cstddef>
#include <limits>
#include <utility>

#ifdef _WIN32

#include <windows.h>

#else // POSIX

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#endif

#endif // POSIX

namespace cuti
{

#ifdef _WIN32

namespace // anonymous
{

struct pidfile_handle_t
{
  pidfile_handle_t(std::string const& path)
  : handle_(CreateFile(path.c_str(),
                       GENERIC_WRITE,
                       0,
                       nullptr,
                       CREATE_NEW,
                       FILE_ATTRIBUTE_NORMAL,
                       nullptr))
  {
    if(handle_ == INVALID_HANDLE_VALUE)
    {
      int cause = last_system_error();
      system_exception_builder_t builder;
      builder <<  "Failed to create PID file " << path;
      builder.explode(cause);
    }
  }

  pidfile_handle_t(pidfile_handle_t const&) = delete;
  pidfile_handle_t& operator=(pidfile_handle_t const&) = delete;

  void write(char const* first, char const* last) noexcept
  {
    assert(first <= last);

    while(first != last)
    {
      constexpr size_t max = std::numeric_limits<DWORD>::max();
      std::size_t to_write = last - first;
      if(to_write > max)
      {
        to_write = max;
      }

      DWORD written;
      BOOL r = WriteFile(
        handle_, first, static_cast<DWORD>(to_write), &written, nullptr);
      if(!r)
      {
        int cause = last_system_error();
        system_exception_builder_t builder;
        builder <<  "Failed to write to PID file";
        builder.explode(cause);
      }

      first += written;
    }
  }
    
  ~pidfile_handle_t()
  {
    CloseHandle(handle_);
  }
      
private :
  HANDLE handle_;
};

} // anonymous

int current_process_id() noexcept
{
  return GetCurrentProcessId();
}

#else // POSIX

namespace // anomymous
{

struct pidfile_handle_t
{
  pidfile_handle_t(std::string const& path)
  : fd_(::open(path.c_str(), O_CREAT | O_EXCL | O_WRONLY | O_CLOEXEC, 0444))
  {
    if(fd_ == -1)
    {
      int cause = last_system_error();
      system_exception_builder_t builder;
      builder <<  "Failed to create PID file " << path;
      builder.explode(cause);
    }
  }

  pidfile_handle_t(pidfile_handle_t const&) = delete;
  pidfile_handle_t& operator=(pidfile_handle_t const&) = delete;

  void write(char const* first, char const* last) noexcept
  {
    assert(first <= last);
    
    while(first != last)
    {
      constexpr std::size_t max = std::numeric_limits<int>::max();
      std::size_t to_write = last - first;
      if(to_write > max)
      {
        to_write = max;
      }

      int r = ::write(fd_, first, static_cast<int>(to_write));
      if(r == -1)
      {
        int cause = last_system_error();
        system_exception_builder_t builder;
        builder <<  "Failed to write to PID file";
        builder.explode(cause);
      }

      first += r;
    }
  }

  ~pidfile_handle_t()
  {
    ::close(fd_);
  }

private :
  int fd_;
};

} // anonymous

int current_process_id() noexcept
{
  return getpid();
}

#endif // POSIX

pidfile_t::pidfile_t(char const* path)
: pidfile_t(path, current_process_id())
{ }

pidfile_t::pidfile_t(char const* path, int pid)
: path_(absolute_path(path))
{
  auto contents = std::to_string(pid);
#ifdef _WIN32
  contents += "\r\n";
#else
  contents += "\n";
#endif

  bool delete_file = false;
  auto file_guard = make_scoped_guard([&]
  {
    if(delete_file)
    {
      try_delete(path_.c_str());
    }
  });
  
  {
    pidfile_handle_t handle(path_);
    delete_file = true;
    handle.write(contents.data(), contents.data() + contents.size());
    delete_file = false;
  }
}

pidfile_t::~pidfile_t()
{
  try_delete(path_.c_str());
}

} // cuti
