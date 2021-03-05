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

#ifndef LOGGER_HPP_
#define LOGGER_HPP_

#include <string>

namespace xes
{

enum class loglevel_t { error, warning, info, debug };

char const *loglevel_string(loglevel_t level);

struct logger_t
{
  logger_t()
  { }

  logger_t(logger_t const&) = delete;
  logger_t& operator=(logger_t const&) = delete;

  void report(loglevel_t level, char const* message)
  {
    this->do_report(level, message);
  }

  void report(loglevel_t level, std::string const& message)
  {
    this->do_report(level, message.c_str());
  }

  virtual ~logger_t()
  { }

private :
  virtual void do_report(loglevel_t level, char const* message) = 0;
};

} // namespace xes

#endif
