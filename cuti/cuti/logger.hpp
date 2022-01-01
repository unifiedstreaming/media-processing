/*
 * Copyright (C) 2021-2022 CodeShop B.V.
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

#ifndef CUTI_LOGGER_HPP_
#define CUTI_LOGGER_HPP_

#include "linkage.h"
#include "chrono_types.hpp"
#include "loglevel.hpp"

#include <cstring>
#include <memory>
#include <mutex>
#include <string>

namespace cuti
{

struct logging_backend_t;

struct CUTI_ABI logger_t
{
  // Nullptr results in no backend (silent logger)
  explicit logger_t(char const* argv0);
  explicit logger_t(std::unique_ptr<logging_backend_t> backend);

  logger_t(logger_t const&) = delete;
  logger_t& operator=(logger_t const&) = delete;

  // Nullptr results in no backend (silent logger)
  void set_backend(std::unique_ptr<logging_backend_t> backend);

  void report(loglevel_t level,
              char const* begin_msg, char const* end_msg);

  void report(loglevel_t level, char const* msg)
  {
    this->report(level, msg, msg + std::strlen(msg));
  }

  void report(loglevel_t level, std::string const& msg)
  {
    this->report(level, msg.data(), msg.data() + msg.length());
  }

  ~logger_t();

private :
  std::mutex mutex_;
  std::unique_ptr<logging_backend_t> backend_;
  unsigned int n_failures_;
  time_point_t first_failure_time_;
  std::string first_failure_reason_;
};

} // namespace cuti

#endif
