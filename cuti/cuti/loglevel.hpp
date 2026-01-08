/*
 * Copyright (C) 2021-2026 CodeShop B.V.
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

#ifndef CUTI_LOGLEVEL_HPP_
#define CUTI_LOGLEVEL_HPP_

#include "args_reader.hpp"
#include "linkage.h"

namespace cuti
{

enum class loglevel_t { error, warning, info, debug };

CUTI_ABI
char const *loglevel_string(loglevel_t level);

// Enable option value parsing for loglevel_t
CUTI_ABI
void parse_optval(char const* name, args_reader_t const& reader,
                  char const* in, loglevel_t& out);

} // cuti

#endif
