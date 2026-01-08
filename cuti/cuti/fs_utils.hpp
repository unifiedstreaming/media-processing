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

#ifndef CUTI_FS_UTILS_HPP_
#define CUTI_FS_UTILS_HPP_

#include "linkage.h"

#include <cassert>
#include <cstddef>
#include <memory>
#include <string>

namespace cuti
{

struct args_reader_t;

struct CUTI_ABI text_output_file_t
{
  text_output_file_t();

  text_output_file_t(text_output_file_t const&) = delete;
  text_output_file_t& operator=(text_output_file_t const&) = delete;

  virtual std::size_t size() const noexcept = 0;
  virtual void write(char const* first, char const* last) = 0;

  virtual ~text_output_file_t();
};

struct CUTI_ABI absolute_path_t
{
  absolute_path_t()
  : value_()
  { }

  explicit absolute_path_t(char const* path);
  explicit absolute_path_t(std::string const& path);

  bool empty() const
  {
    return value_.empty();
  }

  std::string const& value() const
  {
    assert(!this->empty());
    return value_;
  }

  void clear()
  {
    value_.clear();
  }

private :
  std::string value_;
};

// Enable option value parsing for absolute_path_t
CUTI_ABI
void parse_optval(char const* name, args_reader_t const& reader,
                  char const* in, absolute_path_t& out);

CUTI_ABI
std::unique_ptr<text_output_file_t> create_logfile(std::string path);

CUTI_ABI
std::unique_ptr<text_output_file_t> create_pidfile(std::string path);

// Returns 0 on success, and a system error code on failure
CUTI_ABI int try_delete(char const* name) noexcept;

CUTI_ABI void rename_if_exists(char const* old_name, char const* new_name);
CUTI_ABI void delete_if_exists(char const* name);

CUTI_ABI std::string current_directory();
CUTI_ABI void change_directory(char const* name);

} // cuti

#endif
