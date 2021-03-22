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

#include "fs_utils.hpp"
#include "system_error.hpp"

#include <cassert>
#include <string>
#include <vector>

#ifdef _WIN32

#include <windows.h>

namespace xes
{

void rename_if_exists(char const* old_name, char const* new_name)
{
  BOOL result = MoveFile(old_name, new_name);
  if(!result)
  {
    int cause = last_system_error();
    if(cause != ERROR_FILE_NOT_FOUND)
    {
      throw system_exception_t(
        std::string("Can't rename file ") + old_name + " to " + new_name,
        cause);
    }
  }
}

void delete_if_exists(char const* name)
{
  BOOL result = DeleteFile(name);
  if(!result)
  {
    int cause = last_system_error();
    if(cause != ERROR_FILE_NOT_FOUND)
    {
      throw system_exception_t(
        std::string("Can't delete file ") + name,
        cause);
    }
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
    throw system_exception_t(
      std::string("Can't determine absolute path for file ") + path,
      cause);
  }

  assert(length < buffer.size());
  return std::string(buffer.data(), buffer.data() + length);
}

} // xes

#else // POSIX

#include <errno.h>
#include <stdio.h>
#include <unistd.h>

namespace xes
{

void rename_if_exists(char const* old_name, char const* new_name)
{
  int r = ::rename(old_name, new_name);
  if(r == -1)
  {
    int cause = last_system_error();
    if(cause != ENOENT)
    {
      throw system_exception_t(
        std::string("Can't rename file ") + old_name + " to " + new_name,
        cause);
    }
  }
}

void delete_if_exists(char const* name)
{
  int r = ::remove(name);
  if(r == -1)
  {
    int cause = last_system_error();
    if(cause != ENOENT)
    {
      throw system_exception_t(
        std::string("Can't delete file ") + name, cause);
    }
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

std::string absolute_path(char const* path)
{
  if(*path == '\0')
  {
    throw system_exception_t("Can't convert empty path to absolute path");
  }

  std::string result;

  if(*path == '/')
  {
    result = "/";
    ++path;
  }
  else
  {
    result = current_directory();

    assert(!result.empty());
    assert(result.front() == '/');
    if(result.size() > 1)
    {
      assert(result.back() != '/');
    }
  }

  while(*path != '\0')
  {
    // skip leading slashes from path
    while(*path == '/')
    {
      ++path;
      if(*path == '\0')
      {
        // preserve trailing slash
	if(result.back() != '/')
	{
 	  result.push_back('/');
	}
      }
    }

    // find end of segment
    char const* end = path;
    while(*end != '\0' && *end != '/')
    {
      ++end;
    }

    if(end - path == 1 && path[0] == '.')
    {
      // "." -> skip segment in path
    }
    else if(end - path == 2 && path[0] == '.' && path[1] == '.')
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
    else if(end != path)
    {
      // append other non-empty segment from path to result
      if(result.back() != '/')
      {
        result.push_back('/');
      }
      while(path != end)
      {
        result.push_back(*path);
	++path;
      }
    }

    path = end;
  }
    
  return result;    
}
  
} // xes

#endif // POSIX
