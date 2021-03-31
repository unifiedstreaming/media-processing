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

#ifndef XES_DEFAULT_BACKEND_HPP_
#define XES_DEFAULT_BACKEND_HPP_

#include "logging_backend.hpp"

#include <string>
#include <streambuf>

namespace xes
{

/*
 * This is the default backend type a logger uses when no other
 * backend has been set. It is intended as a fallback during early
 * startup before a more sophisticated backend is set.
 */
struct default_backend_t : logging_backend_t
{
  explicit default_backend_t(char const* argv0);

  void report(loglevel_t level,
              char const* begin_msg, char const* end_msg) override;

private :
  std::string const argv0_;
  std::streambuf* const sb_;
};

} // xes

#endif
