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

#include "loglevel.hpp"

#include "system_error.hpp"

#include <cstring>
#include <stdexcept>

namespace cuti
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

void parse_optval(char const* name, args_reader_t const& reader,
                  char const* in, loglevel_t& out)
{
  if(std::strcmp(in, "error") == 0)
  {
    out = loglevel_t::error;
  }
  else if(std::strcmp(in, "warning") == 0)
  {
    out = loglevel_t::warning;
  }
  else if(std::strcmp(in, "info") == 0)
  {
    out = loglevel_t::info;
  }
  else if(std::strcmp(in, "debug") == 0)
  {
    out = loglevel_t::debug;
  }
  else
  {
    system_exception_builder_t builder;
    builder << reader.current_origin() <<
      ": unexpected value '" << in << "' for option '" << name <<
      "'; valid values are 'error', 'warning', 'info' and 'debug'";
    builder.explode();
  }
}

} // cuti
