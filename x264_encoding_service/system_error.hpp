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

#ifndef SYSTEM_ERROR_HPP_
#define SYSTEM_ERROR_HPP_

#include <stdexcept>
#include <string>

namespace xes
{

int last_system_error();
std::string system_error_string(int error);

struct system_exception_t : std::runtime_error
{
  explicit system_exception_t(std::string complaint);
  explicit system_exception_t(std::string const& complaint, int cause);
};

} // namespace xes

#endif
