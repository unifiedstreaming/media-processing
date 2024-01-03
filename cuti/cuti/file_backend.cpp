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

#include "file_backend.hpp"

#include "fs_utils.hpp"
#include "membuf.hpp"
#include "streambuf_backend.hpp"

#include <cassert>
#include <utility>

namespace cuti
{

namespace // anonymous
{

void write_log_entry(text_output_file_t& handle,
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

file_backend_t::file_backend_t(absolute_path_t path,
                               unsigned int size_limit,
                               unsigned int rotation_depth)
: path_((assert(!path.empty()), std::move(path)))
, size_limit_(size_limit)
, rotation_depth_(rotation_depth)
, rotate_reported_(false)
{
  // throw if the logfile cannot be opened, closing it immediately
  auto test_access = create_logfile(path_.value());
}

void file_backend_t::report(loglevel_t level,
                            char const* begin_msg, char const* end_msg)
{
  auto handle = open_log_handle();
  write_log_entry(*handle, level, begin_msg, end_msg);
}

std::unique_ptr<text_output_file_t>
file_backend_t::open_log_handle()
{
  auto result = create_logfile(path_.value());
  if(size_limit_ != 0 && result->size() >= size_limit_)
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
    rotate(path_.value(), rotation_depth_);
    rotate_reported_ = false;

    result = create_logfile(path_.value());
  }

  return result;
}

} // namespace cuti
