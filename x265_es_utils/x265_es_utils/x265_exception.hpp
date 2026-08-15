/*
 * Copyright (C) 2026 CodeShop B.V.
 *
 * This file is part of the x265_es_utils library.
 *
 * The x265_es_utils library is free software: you can redistribute it
 * and/or modify it under the terms of version 2 of the GNU General
 * Public License as published by the Free Software Foundation.
 *
 * The x265_es_utils library is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See version 2 of the GNU General Public License for more details.
 *
 * You should have received a copy of version 2 of the GNU General
 * Public License along with the x265_es_utils library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef X265_ES_UTILS_X265_EXCEPTION_HPP_
#define X265_ES_UTILS_X265_EXCEPTION_HPP_

#include <cuti/exception_builder.hpp>

namespace x265_es_utils
{

struct x265_exception_t : std::runtime_error
{
  explicit x265_exception_t(std::string complaint);
  ~x265_exception_t() override;
};

using x265_exception_builder_t = cuti::exception_builder_t<x265_exception_t>;

} // x265_es_utils

#endif
