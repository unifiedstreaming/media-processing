/*
 * Copyright (C) 2021-2026 CodeShop B.V.
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

struct user_t::impl_t
{
  explicit impl_t(uid_t uid);
  explicit impl_t(char const *name);

  impl_t(impl_t const&) = delete;
  impl_t& operator=(impl_t const&) = delete;

  auto user_id() const
  { return pwd_.pw_uid; }

  auto primary_group_id() const
  { return pwd_.pw_gid; }

  char const* name() const
  { return pwd_.pw_name; }

  void apply() const;

private :
  struct passwd pwd_;
  std::vector<char> buffer_;
};

user_t::impl_t::impl_t(uid_t uid)
: pwd_()
, buffer_(256)
{
  struct passwd *pwd_ptr;

  int r = ::getpwuid_r(uid, &pwd_, buffer_.data(), buffer_.size(), &pwd_ptr);
  while(r == ERANGE)
  {
    auto old_size = buffer_.size();
    buffer_.resize(old_size + old_size / 2);
    r = ::getpwuid_r(uid, &pwd_, buffer_.data(), buffer_.size(), &pwd_ptr);
  }

  if(r != 0)
  {
    system_exception_builder_t builder;
    builder << "getpwuid_r() failure: " << error_status_t(r);
    builder.explode();
  }

  if(pwd_ptr == nullptr)
  {
    system_exception_builder_t builder;
    builder << "unknown user id " << uid;
    builder.explode();
  }

  assert(pwd_ptr == &pwd_);
}

user_t::impl_t::impl_t(char const* name)
: pwd_()
, buffer_(256)
{
  assert(name != nullptr);
  
  struct passwd *pwd_ptr;

  int r = ::getpwnam_r(name, &pwd_, buffer_.data(), buffer_.size(), &pwd_ptr);
  while(r == ERANGE)
  {
    auto old_size = buffer_.size();
    buffer_.resize(old_size + old_size / 2);
    r = ::getpwnam_r(name, &pwd_, buffer_.data(), buffer_.size(), &pwd_ptr);
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

  assert(pwd_ptr == &pwd_);
}

void user_t::impl_t::apply() const
{
  // Set supplementary group ids found in the groups file/database
  int r = ::initgroups(this->name(), this->primary_group_id());
  if(r == -1)
  {
    int cause = last_system_error();
    system_exception_builder_t builder;
    builder << "can't set supplementary group ids for user '" <<
      this->name() << "': " << error_status_t(cause);
    builder.explode();
  }
  
  // Set real and effective primary group ids
  auto gid = this->primary_group_id();
  r = ::setregid(gid, gid);
  if(r == -1)
  {
    int cause = last_system_error();
    system_exception_builder_t builder;
    builder << "can't set primary group id: user '" << this->name() <<
      "' gid " << gid << ": " << error_status_t(cause);
    builder.explode();
  }

  // Set real and effective user ids
  auto uid = this->user_id();
  r = ::setreuid(uid, uid);
  if(r == -1)
  {
    int cause = last_system_error();
    system_exception_builder_t builder;
    builder << "can't set user id: user '" << this->name() << "' uid " <<
      uid << ": " << error_status_t(cause);
    builder.explode();
  }
}
  
unsigned int user_t::user_id() const
{
  assert(!this->empty());
  return impl_->user_id();
}

unsigned int user_t::primary_group_id() const
{
  assert(!this->empty());
  return impl_->primary_group_id();
}

char const* user_t::name() const
{
  assert(!this->empty());
  return impl_->name();
}

void user_t::apply() const
{
  assert(!this->empty());
  impl_->apply();
}

user_t user_t::root()
{
  return user_t(std::make_shared<impl_t>(0));
}

user_t user_t::current()
{
  return user_t(std::make_shared<impl_t>(::geteuid()));
}

user_t user_t::resolve(char const* name)
{
  assert(name != nullptr);
  return user_t(std::make_shared<impl_t>(name));
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
                  char const* in, user_t& out)
{
  try
  {
    out = user_t::resolve(in);
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
