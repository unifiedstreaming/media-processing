/*
 * Copyright (C) 2021-2025 CodeShop B.V.
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

#ifndef CUTI_SYSLOG_BACKEND_HPP_
#define CUTI_SYSLOG_BACKEND_HPP_

#include "linkage.h"
#include "logging_backend.hpp"

#include <string>
#include <memory>

namespace cuti
{

struct CUTI_ABI syslog_backend_t : logging_backend_t
{
  explicit syslog_backend_t(std::string const& source_name);

  void report(loglevel_t level,
              char const* begin_msg, char const* end_msg) override;

  ~syslog_backend_t();

private :
  struct impl_t;
  std::unique_ptr<impl_t> impl_;
};

/*
 * Returns the default service name used for the system log.
 */
CUTI_ABI
std::string default_syslog_name(char const *argv0);

CUTI_ABI
inline std::string default_syslog_name(std::string const& argv0)
{
  return default_syslog_name(argv0.c_str());
}

} // namespace cuti

#endif
