/*
 * Copyright (C) 2021 CodeShop B.V.
 *
 * This file is part of the cuti library.
 *
 * The cuti library is free software: you can redistribute it
 * and/or modify it under the terms of version 2 of the GNU General
 * Public License as published by the Free Software Foundation.
 *
 * The cuti library is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See version 2 of the GNU General Public License for more details.
 *
 * You should have received a copy of version 2 of the GNU General
 * Public License along with the cuti library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef CUTI_FORMAT_HPP_
#define CUTI_FORMAT_HPP_

#include "linkage.h"
#include "logger.hpp"

#include <chrono>
#include <streambuf>

namespace cuti
{

CUTI_ABI void format_unsigned(std::streambuf& target, unsigned int number,
                              int width = 0);
CUTI_ABI void format_string(std::streambuf& target, const char* str,
                            int width = 0);
CUTI_ABI void format_loglevel(std::streambuf& target, loglevel_t level);
CUTI_ABI void format_timepoint(std::streambuf& target,
                               std::chrono::system_clock::time_point tp);

} // namespace cuti

#endif
