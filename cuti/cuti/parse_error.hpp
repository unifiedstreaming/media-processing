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

#ifndef CUTI_PARSE_ERROR_HPP_
#define CUTI_PARSE_ERROR_HPP_

#include "linkage.h"

#include <stdexcept>
#include <string>
#include <utility>

namespace cuti
{

struct CUTI_ABI parse_error_t : std::runtime_error
{
  explicit parse_error_t(std::string message)
  : std::runtime_error(std::move(message))
  { }

  ~parse_error_t() override;
};

} // cuti

#endif
