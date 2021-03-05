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

#include "logger.hpp"

namespace xes
{

char const* loglevel_string(loglevel_t level)
{
  switch(level)
  {
  case loglevel_t::error : return "error";
  case loglevel_t::warning : return "warning";
  case loglevel_t::info: return "info";
  case loglevel_t::debug: return "debug";
  }

  return "<invalid log level>";
}

} // namespace xes
