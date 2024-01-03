/*
 * Copyright (C) 2021-2024 CodeShop B.V.
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

#include "process_utils.hpp"

#include "args_reader.hpp"
#include "fs_utils.hpp"
#include "scoped_guard.hpp"
#include "system_error.hpp"

#include <cassert>
#include <utility>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <grp.h>
#include <pwd.h>
#include <unistd.h>
#endif

namespace cuti
{

#ifdef _WIN32

int current_process_id() noexcept
{
  return GetCurrentProcessId();
}

#else // POSIX

int current_process_id() noexcept
{
  return getpid();
}

umask_t umask_t::apply() const
{
  auto prev_umask = ::umask(this->value());
  return umask_t(prev_umask);
}

void user_id_t::apply() const
{
  int r = ::setuid(this->value());
  if(r == -1)
  {
    int cause = last_system_error();
    system_exception_builder_t builder;
    builder << "can\'t set user id to " << this->value() << ": " <<
      error_status_t(cause);
    builder.explode();
  }
}

user_id_t user_id_t::current() noexcept
{
  auto value = ::geteuid();
  return user_id_t(value);
}
  
user_id_t user_id_t::resolve(char const* name)
{
  std::vector<char> buffer(256);
  struct passwd pwd;
  struct passwd *pwd_ptr;

  int r = ::getpwnam_r(name, &pwd, buffer.data(), buffer.size(), &pwd_ptr);
  while(r == ERANGE)
  {
    buffer.resize(buffer.size() * 2);
    r = ::getpwnam_r(name, &pwd, buffer.data(), buffer.size(), &pwd_ptr);
  }

  if(r != 0)
  {
    system_exception_builder_t builder;
    builder << "getpwnam_r() failure: " << error_status_t(r);
    builder.explode();
  }

  if(pwd_ptr == nullptr)
  {
    system_exception_builder_t builder;
    builder << "unknown user name '" << name << "'";
    builder.explode();
  }

  assert(pwd_ptr == &pwd);
  
  return user_id_t(pwd.pw_uid);
}

void group_id_t::apply() const
{
  int r = ::setgid(this->value());
  if(r == -1)
  {
    int cause = last_system_error();
    system_exception_builder_t builder;
    builder << "can\'t set group id to " << this->value() << ": " <<
      error_status_t(cause);
    builder.explode();
  }
}

group_id_t group_id_t::current() noexcept
{
  auto value = ::getegid();
  return group_id_t(value);
}
  
group_id_t group_id_t::resolve(char const* name)
{
  std::vector<char> buffer(256);
  struct group grp;
  struct group *grp_ptr;

  int r = ::getgrnam_r(name, &grp, buffer.data(), buffer.size(), &grp_ptr);
  while(r == ERANGE)
  {
    buffer.resize(buffer.size() * 2);
    r = ::getgrnam_r(name, &grp, buffer.data(), buffer.size(), &grp_ptr);
  }

  if(r != 0)
  {
    system_exception_builder_t builder;
    builder << "getgrnam_r() failure: " << error_status_t(r);
    builder.explode();
  }

  if(grp_ptr == nullptr)
  {
    system_exception_builder_t builder;
    builder << "unknown group name '" << name << "'";
    builder.explode();
  }

  assert(grp_ptr == &grp);
  
  return group_id_t(grp.gr_gid);
}

void parse_optval(char const* name, args_reader_t const& reader,
                  char const* in, umask_t& out)
{
  int value = 0;
  char max_digit = '0';
  do
  {
    if(*in < '0' || *in > max_digit)
    {
      system_exception_builder_t builder;
      builder << reader.current_origin() <<
        ": value for option '" << name <<
        "' must consist of octal digits and start with a '0'";
      builder.explode();
    }

    int dval = *in - '0';
    if(value > umask_t::maximum / 8 || dval > umask_t::maximum - value * 8)
    {
      system_exception_builder_t builder;
      builder << reader.current_origin() <<
        ": overflow in value for option '" << name << "'";
      builder.explode();
    }

    value *= 8;
    value += dval;
    max_digit = '7';

    ++in;
  } while(*in != '\0');

  out = umask_t(value);
}

void parse_optval(char const* name, args_reader_t const& reader,
                  char const* in, user_id_t& out)
{
  try
  {
    out = user_id_t::resolve(in);
  }
  catch(std::exception const& ex)
  {
    system_exception_builder_t builder;
    builder << reader.current_origin() <<
      ": option '" << name << "': " << ex.what();
    builder.explode();
  }
}

void parse_optval(char const* name, args_reader_t const& reader,
                  char const* in, group_id_t& out)
{
  try
  {
    out = group_id_t::resolve(in);
  }
  catch(std::exception const& ex)
  {
    system_exception_builder_t builder;
    builder << reader.current_origin() <<
      ": option '" << name << "': " << ex.what();
    builder.explode();
  }
}

#endif // POSIX

pidfile_t::pidfile_t(absolute_path_t path)
: pidfile_t(std::move(path), current_process_id())
{ }

pidfile_t::pidfile_t(absolute_path_t path, int pid)
: path_((assert(!path.empty()), std::move(path)))
{
  auto contents = std::to_string(pid) + '\n';

  auto handle = create_pidfile(path_.value());
  auto file_guard = make_scoped_guard([&]
  {
    handle.reset();
    try_delete(path_.value().c_str());
  });
  
  handle->write(contents.data(), contents.data() + contents.size());
  file_guard.dismiss();
}

pidfile_t::~pidfile_t()
{
  try_delete(path_.value().c_str());
}

} // cuti
