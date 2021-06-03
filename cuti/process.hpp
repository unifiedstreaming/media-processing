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

#ifndef CUTI_PROCESS_HPP_
#define CUTI_PROCESS_HPP_

#include "linkage.h"

#include <cassert>
#include <string>

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
  explicit pidfile_t(char const* path);
  pidfile_t(char const* path, int pid);

  pidfile_t(pidfile_t const&);
  pidfile_t& operator=(pidfile_t const&);
  
  std::string const& effective_filename() const noexcept
  { return path_; }

  ~pidfile_t();

private :
  std::string const path_;
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

struct CUTI_ABI user_id_t
{
  user_id_t() noexcept
  : value_(0)
  { }

  explicit user_id_t(unsigned int value) noexcept
  : value_(value)
  { }

  unsigned int value() const noexcept
  { return value_; }

  /*
   * Sets the effective user id of the current process to *this.
   */
  void apply() const;

  /*
   * Returns the effective user id of the current process.
   */
  static user_id_t current() noexcept;

  /*
   * Resolves a user name.
   */
  static user_id_t resolve(char const* name);

private :
  unsigned int value_;
};

struct CUTI_ABI group_id_t
{
  group_id_t() noexcept
  : value_(0)
  { }

  explicit group_id_t(unsigned int value) noexcept
  : value_(value)
  { }

  unsigned int value() const noexcept
  { return value_; }

  /*
   * Sets the effective user id of the current process to *this.
   */
  void apply() const;

  /*
   * Returns the effective group id of the current process.
   */
  static group_id_t current() noexcept;

  /*
   * Resolves a group name.
   */
  static group_id_t resolve(char const* name);

private :
  unsigned int value_;
};

// Enable option value parsing for umask_t, user_id_t, group_id_t
CUTI_ABI
void parse_optval(char const* name, args_reader_t const& reader,
                  char const* in, umask_t& out);

CUTI_ABI
void parse_optval(char const* name, args_reader_t const& reader,
                  char const* in, user_id_t& out);

CUTI_ABI
void parse_optval(char const* name, args_reader_t const& reader,
                  char const* in, group_id_t& out);

#endif // POSIX

} // cuti

#endif
