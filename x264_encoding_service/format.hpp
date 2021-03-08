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

#ifndef FORMAT_HPP_
#define FORMAT_HPP_

#include "logger.hpp"

#include <chrono>
#include <streambuf>

namespace xes
{

void format_unsigned(std::streambuf& target, unsigned int number,
                     int width = 0);
void format_string(std::streambuf& target, const char* str,
                   int width = 0);
void format_loglevel(std::streambuf& target, loglevel_t level);
void format_timepoint(std::streambuf& target,
                      std::chrono::system_clock::time_point tp);

} // namespace xes

#endif
