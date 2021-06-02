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

#include "fs_utils.hpp"
#include "system_error.hpp"

#include <cassert>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#ifdef _WIN32

#include <windows.h>

#else // POSIX

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#endif

#endif // POSIX

namespace cuti
{

#ifdef _WIN32

namespace // anoymous
{

struct text_output_file_impl_t : text_output_file_t
{
  text_output_file_impl_t(std::string path, HANDLE (*opener)(char const *))
  : text_output_file_t()
  , path_(std::move(path))
  , handle_(opener(path_.c_str()))
  {
    if(handle_ == INVALID_HANDLE_VALUE)
    {
      int cause = last_system_error();
      system_exception_builder_t builder;
      builder <<  "Failed to open file " << path_;
      builder.explode(cause);
    }
  }

  std::size_t size() const noexcept override
  {
    LARGE_INTEGER size;
    BOOL result = GetFileSizeEx(handle_, &size);
    if(!result)
    {
      return 0;
    }
    return static_cast<std::size_t>(size.QuadPart);
  }

  void write(char const* first, char const* last) override
  {
    char const* newline = std::find(first, last, '\n');
    while(newline != last)
    {
      static char const crlf[2] = { '\r', '\n' };

      if(newline == first)
      {
        this->do_write(crlf, crlf + sizeof crlf);
      }
      else if(*(newline - 1) != '\r')
      {
        this->do_write(first, newline);
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

  ~text_output_file_impl_t() override
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
        system_exception_builder_t builder;
        builder <<  "Error writing to file " << path_;
        builder.explode(cause);
      }

      first += written;
    }
  }

private :
  std::string path_;
  HANDLE handle_;
};

HANDLE open_logfile_handle(char const* path)
{
  return CreateFile(path,
                    FILE_APPEND_DATA,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    nullptr,
                    OPEN_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL,
                    nullptr);
}

HANDLE open_pidfile_handle(char const *path)
{
  return CreateFile(path,
                    GENERIC_WRITE,
                    0,
                    nullptr,
                    CREATE_NEW,
                    FILE_ATTRIBUTE_NORMAL,
                    nullptr);
}

} // anonymous

int try_delete(char const* name) noexcept
{
  return DeleteFile(name) ? 0 : last_system_error();
}

void rename_if_exists(char const* old_name, char const* new_name)
{
  BOOL result = MoveFile(old_name, new_name);
  if(!result)
  {
    int cause = last_system_error();
    if(cause != ERROR_FILE_NOT_FOUND)
    {
      system_exception_builder_t builder;
      builder << "Can't rename file " << old_name << " to " << new_name;
      builder.explode(cause);
    }
  }
}

void delete_if_exists(char const* name)
{
  int error = try_delete(name);
  if(error != 0 && error != ERROR_FILE_NOT_FOUND)
  {
    system_exception_builder_t builder;
    builder << "Can't delete file " << name;
    builder.explode(error);
  }
}

std::string current_directory()
{
  std::vector<char> buffer(256);

  DWORD length = GetCurrentDirectory(
    static_cast<DWORD>(buffer.size()), buffer.data());
  if(length >= buffer.size())
  {
    buffer.resize(length);
    length = GetCurrentDirectory(
      static_cast<DWORD>(buffer.size()), buffer.data());
  }

  if(length == 0)
  {
    int cause = last_system_error();
    throw system_exception_t("Can't determine current directory", cause);
  }

  assert(length < buffer.size());
  return std::string(buffer.data(), buffer.data() + length);
}

void change_directory(char const* path)
{
  BOOL r = SetCurrentDirectory(path);
  if(!r)
  {
    int cause = last_system_error();
    system_exception_builder_t builder;
    builder << "Can't change directory to '" << path << "'";
    builder.explode(cause);
  }
}
  
std::string absolute_path(char const* path)
{
  std::vector<char> buffer(256);

  DWORD length = GetFullPathName(
    path, static_cast<DWORD>(buffer.size()), buffer.data(), nullptr);
  if(length >= buffer.size())
  {
    buffer.resize(length);
    length = GetFullPathName(
      path, static_cast<DWORD>(buffer.size()), buffer.data(), nullptr);
  }

  if(length == 0)
  {
    int cause = last_system_error();
    system_exception_builder_t builder;
    builder << "Can't determine absolute path for file " << path;
    builder.explode(cause);
  }

  assert(length < buffer.size());
  return std::string(buffer.data(), buffer.data() + length);
}

#else // POSIX

namespace // anonymous
{

struct text_output_file_impl_t : text_output_file_t
{
  text_output_file_impl_t(std::string path, int (*opener)(char const*))
  : text_output_file_t()
  , path_(std::move(path))
  , fd_(opener(path_.c_str()))
  {
    if(fd_ == -1)
    {
      int cause = last_system_error();
      system_exception_builder_t builder;
      builder << "Failed to open file " << path_;
      builder.explode(cause);
    }
  }

  std::size_t size() const noexcept override
  {
    struct stat statbuf;
    if(::fstat(fd_, &statbuf) == -1 || (statbuf.st_mode & S_IFMT) != S_IFREG)
    {
      return 0;
    }
    return statbuf.st_size;
  }

  void write(char const* first, char const* last) override
  {
    while(first != last)
    {
      auto result = ::write(fd_, first, last - first);
      if(result == -1)
      {
        int cause = last_system_error();
        system_exception_builder_t builder;
        builder <<  "Error writing to file " << path_;
        builder.explode(cause);
      }
      first += result;
    }
  }

  ~text_output_file_impl_t() override
  {
    ::close(fd_);
  }

private :
  std::string path_;
  int fd_;
};

int open_logfile_handle(char const* path)
{
  return ::open(path, O_CREAT | O_WRONLY | O_APPEND | O_CLOEXEC, 0666);
}

int open_pidfile_handle(char const *path)
{
  return ::open(path, O_CREAT | O_EXCL | O_WRONLY | O_CLOEXEC, 0444);
}

} // anonymous

int try_delete(char const* name) noexcept
{
  return ::remove(name) != -1 ? 0 : last_system_error();
}

void rename_if_exists(char const* old_name, char const* new_name)
{
  int r = ::rename(old_name, new_name);
  if(r == -1)
  {
    int cause = last_system_error();
    if(cause != ENOENT)
    {
      system_exception_builder_t builder;
      builder << "Can't rename file " << old_name << " to " << new_name;
      builder.explode(cause);
    }
  }
}

void delete_if_exists(char const* name)
{
  int error = try_delete(name);
  if(error != 0 && error != ENOENT)
  {
    system_exception_builder_t builder;
    builder << "Can't delete file " << name;
    builder.explode(error);
  }
}

std::string current_directory()
{
  std::vector<char> buffer(256);

  char* result;
  while((result = ::getcwd(buffer.data(), buffer.size())) == nullptr)
  {
    int cause = last_system_error();
    if(cause != ERANGE)
    {
      throw system_exception_t("Can't determine current directory", cause);
    }
    buffer.resize(buffer.size() * 2);
  }

  return result;
}

void change_directory(char const* path)
{
  int r = ::chdir(path);
  if(r == -1)
  {
    int cause = last_system_error();
    system_exception_builder_t builder;
    builder << "Can't change directory to '" << path << "'";
    builder.explode(cause);
  }
}
  
std::string absolute_path(char const* path)
{
  std::string result;

  switch(*path)
  {
  case '\0' :
    throw system_exception_t("Can't convert empty path to absolute path");
    break;
  case '/' :
    result = "/";
    break;
  default :
    result = current_directory();
    break;
  }

  while(*path != '\0')
  {
    assert(!result.empty());
    assert(result.front() == '/');
    assert(result.size() == 1 || result.back() != '/');

    // skip past leading slashes
    while(*path == '/')
    {
      ++path;
    }

    // remember start of segment, move path to end of segment
    char const* seg = path;
    while(*path != '\0' && *path != '/')
    {
      ++path;
    }

    if(path - seg == 1 && seg[0] == '.')
    {
      // "." -> skip segment in path
    }
    else if(path - seg == 2 && seg[0] == '.' && seg[1] == '.')
    {
      // ".." -> pop last segment from result
      while(result.size() > 1 && result.back() != '/')
      {
        result.pop_back();
      }
      if(result.size() > 1)
      {
        result.pop_back();
      }
    }
    else
    {
      // append segment (which is only empty on a trailing slash) to result
      if(result.back() != '/')
      {
        result.push_back('/');
      }
      while(seg != path)
      {
        result.push_back(*seg);
        ++seg;
      }
    }
  }

  return result;
}

#endif // POSIX

text_output_file_t::text_output_file_t()
{ }

text_output_file_t::~text_output_file_t()
{ }

std::unique_ptr<text_output_file_t> create_logfile(std::string path)
{
  return std::make_unique<text_output_file_impl_t>(
    std::move(path), open_logfile_handle);
}

std::unique_ptr<text_output_file_t> create_pidfile(std::string path)
{
  return std::make_unique<text_output_file_impl_t>(
    std::move(path), open_pidfile_handle);
}

} // cuti
