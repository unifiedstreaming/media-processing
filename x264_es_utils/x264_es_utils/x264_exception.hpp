/*
 * Copyright (C) 2026 CodeShop B.V.
 *
 * This file is part of the x264_es_utils library.
 *
 * The x264_es_utils library is free software: you can redistribute it
 * and/or modify it under the terms of version 2 of the GNU General
 * Public License as published by the Free Software Foundation.
 *
 * The x264_es_utils library is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See version 2 of the GNU General Public License for more details.
 *
 * You should have received a copy of version 2 of the GNU General
 * Public License along with the x264_es_utils library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef X264_ES_UTILS_X264_EXCEPTION_HPP_
#define X264_ES_UTILS_X264_EXCEPTION_HPP_

#include <cuti/exception_builder.hpp>

namespace x264_es_utils
{

struct x264_exception_t : std::runtime_error
{
  explicit x264_exception_t(std::string complaint);
  ~x264_exception_t() override;
};

using x264_exception_builder_t = cuti::exception_builder_t<x264_exception_t>;

} // x264_es_utils

#endif
