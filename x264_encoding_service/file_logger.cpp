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

#include "file_logger.hpp"

#include "format.hpp"
#include "logbuf.hpp"
#include "system_error.hpp"

#include <limits>
#include <utility>

#ifdef _WIN32

#include <windows.h>

namespace xes
{

struct file_logger_t::log_handle_t
{
  log_handle_t(char const* filename)
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
      throw system_exception_t(
        std::string("Failed to open log file ") + filename, cause);
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

namespace // anonymous
{

void rename_if_exists(std::string const& old_name, std::string const& new_name)
{
  BOOL result = MoveFile(old_name.c_str(), new_name.c_str());
  if(!result)
  {
    int cause = last_system_error();
    if(cause != ERROR_FILE_NOT_FOUND)
    {
      throw system_exception_t(
        "Can't rename file " + old_name + " to " + new_name, cause);
    }
  }
}

void delete_if_exists(std::string const& name)
{
  BOOL result = DeleteFile(name.c_str());
  if(!result)
  {
    int cause = last_system_error();
    if(cause != ERROR_FILE_NOT_FOUND)
    {
      throw system_exception_t("Can't delete file " + name, cause);
    }
  }
}

} // anonymous

} // xes

#else // POSIX

#error TODO: POSIX

#endif

namespace xes
{

namespace // anonymous
{

void write_log_entry(file_logger_t::log_handle_t& handle,
                     loglevel_t level,
                     const char* message)
{
  logbuf_t buffer;

  auto now = std::chrono::system_clock::now();
  format_timepoint(buffer, now);
  buffer.sputc(' ');
  format_loglevel(buffer, level);
  buffer.sputc(' ');
  format_string(buffer, message);
  buffer.sputc('\n');

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
    rename_if_exists(old_name, new_name);
  }
  else
  {
    delete_if_exists(old_name);
  }
}

void rotate(std::string const& name, unsigned int depth)
{
  do_rotate(name, 0, depth);
}

} // anonymous

file_logger_t::file_logger_t(std::string filename,
                             unsigned int size_limit,
                             unsigned int rotation_depth)
: filename_(std::move(filename))
, size_limit_(size_limit)
, rotation_depth_(rotation_depth)
, mutex_()
, rotating_(false)
, n_failures_(0)
, first_failure_time_()
, first_failure_reason_()
{ }

file_logger_t::~file_logger_t()
{ }

void file_logger_t::do_report(loglevel_t level, char const* message)
{
  static auto const max_failures = std::numeric_limits<unsigned int>::max();

  std::lock_guard<std::mutex> lock(mutex_);

  try
  {
    auto handle = open_log_handle();

    // report previous failures first...
    if(n_failures_ != 0)
    {
      logbuf_t buffer;
      format_string(buffer, "Logging failed at ");
      format_timepoint(buffer, first_failure_time_);
      format_string(buffer, ": ");
      format_string(buffer, first_failure_reason_.c_str());

      format_string(buffer, " - ");
      if(n_failures_ != max_failures)
      {
        format_unsigned(buffer, n_failures_);
      }
      else
      {
        format_string(buffer, "many");
      }
      format_string(buffer, " message(s) lost");
      
      write_log_entry(*handle, loglevel_t::error, buffer.c_str());
    }

    // ...then report the current event
    write_log_entry(*handle, level, message);

    // if we're still here, leave failure mode
    n_failures_ = 0;
  }
  catch(system_exception_t const& ex)
  {
    if(n_failures_ == 0)
    {
      // enter failure mode
      first_failure_time_ = std::chrono::system_clock::now();
      first_failure_reason_ = ex.what();
    }
    if(n_failures_ != max_failures)
    {
      ++n_failures_;
    }
  }
}
    
std::unique_ptr<file_logger_t::log_handle_t>
file_logger_t::open_log_handle()
{
  std::unique_ptr<log_handle_t> result(new log_handle_t(filename_.c_str()));
  if(size_limit_ != 0 && result->filesize() >= size_limit_)
  {
    /*
     * Try to add an entry to the old log to say we're rotating, but
     * avoid repeating that entry over and over again when rotation
     * fails.
     */
    if(!rotating_)
    {
      rotating_ = true;
      write_log_entry(*result, loglevel_t::info,
                      "Size limit reached. Rotating...");
    }

    result.reset();
    rotate(filename_, rotation_depth_);
    rotating_ = false;

    result.reset(new log_handle_t(filename_.c_str()));
  }

  return result;
}

} // namespace xes
