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

#include <string>

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

} // xes

#else // POSIX

#include <errno.h>
#include <stdio.h>

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
        std::string("Can't delete file ") + name,
        cause);
    }
  }
}
  
} // xes

#endif // POSIX
