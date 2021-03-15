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

#ifndef LOGGING_BACKEND_HPP_
#define LOGGING_BACKEND_HPP_

#include "logger.hpp"

namespace xes
{

/*
 * Abstract base class for the various logging backends.  A logging
 * backend may assume it is not called concurrently, and is expected
 * to throw a system_exception_t on failure.
 */
struct logging_backend_t
{
  logging_backend_t()
  { }

  logging_backend_t(logging_backend_t const&) = delete;
  logging_backend_t& operator=(logging_backend_t const&) = delete;

  virtual void report(loglevel_t level,
                      char const* begin_msg, char const* end_msg) = 0;

  virtual ~logging_backend_t()
  { }
};
  
} // xes

#endif
