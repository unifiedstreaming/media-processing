/*
 * Copyright (C) 2021-2022 CodeShop B.V.
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

#ifndef CUTI_SYSTEM_ERROR_HPP_
#define CUTI_SYSTEM_ERROR_HPP_

#include <ostream>
#include <stdexcept>
#include <string>

#include "error_status.hpp"
#include "exception_builder.hpp"
#include "linkage.h"

namespace cuti
{

CUTI_ABI int last_system_error();
CUTI_ABI std::string system_error_string(int error);

struct CUTI_ABI system_exception_t : std::runtime_error
{
  explicit system_exception_t(std::string complaint);
  ~system_exception_t() override;
};

using system_exception_builder_t = exception_builder_t<system_exception_t>;

} // namespace cuti

#endif
