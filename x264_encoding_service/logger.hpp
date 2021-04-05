/*
 * Copyright (C) 2021 CodeShop B.V.
 *
 * This file is part of the x264_encoding_service.
 *
 * The x264_encoding_service is free software: you can redistribute it
 * and/or modify it under the terms of version 2 of the GNU General
 * Public License as published by the Free Software Foundation.
 *
 * The x264_encoding_service is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See version 2 of the GNU General Public License for more details.
 *
 * You should have received a copy of version 2 of the GNU General
 * Public License along with the x264_encoding_service.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef XES_LOGGER_HPP_
#define XES_LOGGER_HPP_

#include <chrono>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>

namespace xes
{

enum class loglevel_t { error, warning, info, debug };

char const *loglevel_string(loglevel_t level);

// Enable option value parsing for loglevel_t
void parse_optval(char const* name, char const* in, loglevel_t& out);

struct logging_backend_t;

struct logger_t
{
  explicit logger_t(char const* argv0);

  logger_t(logger_t const&) = delete;
  logger_t& operator=(logger_t const&) = delete;

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
  std::chrono::system_clock::time_point first_failure_time_;
  std::string first_failure_reason_;
};

} // namespace xes

#endif
