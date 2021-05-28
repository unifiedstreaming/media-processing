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

#include <string>

namespace cuti
{

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

} // cuti

#endif
