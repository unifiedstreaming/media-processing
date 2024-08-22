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

#ifndef CUTI_PROCESS_UTILS_HPP_
#define CUTI_PROCESS_UTILS_HPP_

#include "linkage.h"

#include "fs_utils.hpp"

#include <cassert>
#include <memory>
#include <string>
#include <utility>

namespace cuti
{

struct args_reader_t;

CUTI_ABI
int current_process_id() noexcept;

/*
 * PID file holder class; requires that the file does not exist at
 * creation time and attempts to delete the file when destroyed.
 */
struct CUTI_ABI pidfile_t
{
  explicit pidfile_t(absolute_path_t path);
  pidfile_t(absolute_path_t path, int pid);

  pidfile_t(pidfile_t const&) = delete;
  pidfile_t& operator=(pidfile_t const&) = delete;
  
  ~pidfile_t();

private :
  absolute_path_t const path_;
};

#ifndef _WIN32 // POSIX

struct CUTI_ABI umask_t
{
  static int constexpr minimum = 0;
  static int constexpr maximum = 0777;

  umask_t() noexcept
  : value_(0)
  { }

  explicit umask_t(int value) noexcept
  : value_((assert(value >= minimum), assert(value <= maximum), value))
  { }

  int value() const noexcept
  { return value_; }

  /*
   * Applies *this to the current process, returning the previous
   * umask of the current process.
   */
  umask_t apply() const;

private :
  int value_;
};

struct CUTI_ABI user_t
{
  user_t()
  : impl_(nullptr)
  { }

  bool empty() const
  { return impl_ == nullptr; }

  // PRE: !empty()
  unsigned int user_id() const;
  unsigned int primary_group_id() const;
  char const* name() const;
  void apply() const;

  // These 'named constructors' are guaranteed to either
  // return a non-empty user_t or throw.
  static user_t root();
  static user_t current();
  static user_t resolve(char const* name);

private :
  struct impl_t;

  explicit user_t(std::shared_ptr<impl_t const> impl)
  : impl_(std::move(impl))
  { }

private : 
  std::shared_ptr<impl_t const> impl_;
};

// Enable option value parsing for umask_t, user_t
CUTI_ABI
void parse_optval(char const* name, args_reader_t const& reader,
                  char const* in, umask_t& out);

CUTI_ABI
void parse_optval(char const* name, args_reader_t const& reader,
                  char const* in, user_t& out);

#endif // POSIX

} // cuti

#endif
