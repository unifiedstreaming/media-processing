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

#include "file_backend.hpp"

#include "fs_utils.hpp"
#include "membuf.hpp"
#include "streambuf_backend.hpp"
#include "system_error.hpp"

#include <limits>

#ifdef _WIN32

#include <windows.h>

namespace cuti
{

struct file_backend_t::log_handle_t
{
  explicit log_handle_t(char const* filename)
  : handle_(CreateFile(filename,
                       FILE_APPEND_DATA,
                       FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                       nullptr,
                       OPEN_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL,
                       nullptr))
  {
    if(handle_ == INVALID_HANDLE_VALUE)
    {
      int cause = last_system_error();
      system_exception_builder_t builder;
      builder <<  "Failed to open log file " << filename;
      builder.explode(cause);
    }
  }
    
  log_handle_t(log_handle_t const&);
  log_handle_t& operator=(log_handle_t const&);

  std::size_t filesize() const noexcept
  {
    LARGE_INTEGER size;
    BOOL result = GetFileSizeEx(handle_, &size);
    if(!result)
    {
      return 0;
    }
    return static_cast<std::size_t>(size.QuadPart);
  }
  
  void write(char const* first, char const* last)
  {
    char const* newline = std::find(first, last, '\n');
    while(newline != last)
    {
      if(newline == first || *(newline - 1) != '\r')
      {
        this->do_write(first, newline);
        static char const crlf[2] = { '\r', '\n' };
        this->do_write(crlf, crlf + sizeof crlf);
      }
      else
      {
        this->do_write(first, newline + 1);
      }

      first = newline + 1;
      newline = std::find(first, last, '\n');
    }

    this->do_write(first, last);
  }

  ~log_handle_t()
  {
    CloseHandle(handle_);
  }

private :
  void do_write(char const* first, char const* last)
  {
    while(first != last)
    {
      std::size_t to_write = last - first;
      static size_t const max = std::numeric_limits<DWORD>::max();
      if(to_write > max)
      {
        to_write = max;
      }
      
      DWORD written;
      BOOL result = WriteFile(handle_, first, static_cast<DWORD>(to_write),
                              &written, nullptr);
      if(!result)
      {
        int cause = last_system_error();
        throw system_exception_t("WriteFile() failure", cause);
      }

      first += written;
    }
  }

private :
  HANDLE handle_;
};

} // cuti

#else // POSIX

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#endif

namespace cuti
{

struct file_backend_t::log_handle_t
{
  explicit log_handle_t(char const* filename)
  : fd_(::open(filename, O_CREAT | O_WRONLY | O_APPEND | O_CLOEXEC, 0666))
  {
    if(fd_ == -1)
    {
      int cause = last_system_error();
      system_exception_builder_t builder;
      builder << "Failed to open log file " << filename;
      builder.explode(cause);
    }
  }

  log_handle_t(log_handle_t const&) = delete;
  log_handle_t& operator=(log_handle_t const&) = delete;

  std::size_t filesize() const noexcept
  {
    struct stat statbuf;
    if(::fstat(fd_, &statbuf) == -1 || (statbuf.st_mode & S_IFMT) != S_IFREG)
    {
      return 0;
    }
    return statbuf.st_size;
  }
    
  void write(char const* first, char const* last)
  {
    while(first != last)
    {
      auto result = ::write(fd_, first, last - first);
      if(result == -1)
      {
        int cause = last_system_error();
        throw system_exception_t("write() failure", cause);
      }
      first += result;
    }
  }
        
  ~log_handle_t()
  {
    ::close(fd_);
  }

private :
  int fd_;
};

} // cuti

#endif // POSIX

namespace cuti
{

namespace // anonymous
{

void write_log_entry(file_backend_t::log_handle_t& handle,
                     loglevel_t level,
                     char const* begin_msg, char const* end_msg)
{
  membuf_t buffer;
  streambuf_backend_t delegate(&buffer);
  delegate.report(level, begin_msg, end_msg);
  handle.write(buffer.begin(), buffer.end());
}
  
void do_rotate(std::string const& name, unsigned int level, unsigned int depth)
{
  std::string old_name = name;
  if(level != 0)
  {
    old_name += '.';
    old_name += std::to_string(level);
  }

  if(level != depth)
  {
    do_rotate(name, level + 1, depth);
    std::string new_name = name + '.' + std::to_string(level + 1);
    rename_if_exists(old_name.c_str(), new_name.c_str());
  }
  else
  {
    delete_if_exists(old_name.c_str());
  }
}

void rotate(std::string const& name, unsigned int depth)
{
  do_rotate(name, 0, depth);
}

} // anonymous

file_backend_t::file_backend_t(std::string const& filename,
                               unsigned int size_limit,
                               unsigned int rotation_depth)
: filename_(absolute_path(filename.c_str()))
, size_limit_(size_limit)
, rotation_depth_(rotation_depth)
, rotate_reported_(false)
{ }

void file_backend_t::report(loglevel_t level,
                            char const* begin_msg, char const* end_msg)
{
  auto handle = open_log_handle();
  write_log_entry(*handle, level, begin_msg, end_msg);
}
    
std::unique_ptr<file_backend_t::log_handle_t>
file_backend_t::open_log_handle()
{
  std::unique_ptr<log_handle_t> result(new log_handle_t(filename_.c_str()));
  if(size_limit_ != 0 && result->filesize() >= size_limit_)
  {
    /*
     * Try to add an entry to the old log to say we're rotating, but
     * avoid repeating that entry while rotation keeps throwing.
     */
    if(!rotate_reported_)
    {
      static char const message[] = "Size limit reached. Rotating..."; 
      write_log_entry(*result, loglevel_t::info,
                      message, message + sizeof message - 1);
      rotate_reported_ = true;
    }

    result.reset();
    rotate(filename_, rotation_depth_);
    rotate_reported_ = false;

    result.reset(new log_handle_t(filename_.c_str()));
  }

  return result;
}

} // namespace cuti
