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

#include "process.hpp"

#include "fs_utils.hpp"
#include "scoped_guard.hpp"

#ifdef _WIN32
#include <windows.h>
#else
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

#endif // POSIX

pidfile_t::pidfile_t(char const* path)
: pidfile_t(path, current_process_id())
{ }

pidfile_t::pidfile_t(char const* path, int pid)
: path_(absolute_path(path))
{
  auto contents = std::to_string(pid) + '\n';

  bool delete_file = false;
  auto file_guard = make_scoped_guard([&]
  {
    if(delete_file)
    {
      try_delete(path_.c_str());
    }
  });
  
  {
    auto handle = create_pidfile(path);
    delete_file = true;
    handle->write(contents.data(), contents.data() + contents.size());
    delete_file = false;
  }
}

pidfile_t::~pidfile_t()
{
  try_delete(path_.c_str());
}

} // cuti
