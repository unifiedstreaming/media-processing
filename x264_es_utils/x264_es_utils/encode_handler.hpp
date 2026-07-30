/*
 * Copyright (C) 2024-2026 CodeShop B.V.
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

#ifndef X264_ES_UTILS_ENCODE_HANDLER_HPP_
#define X264_ES_UTILS_ENCODE_HANDLER_HPP_

#include "encoder_settings.hpp"
#include "encoding_session.hpp"

#include <x264_proto/types.hpp>
#include <x26x_es_utils/encode_handler.hpp>

namespace x264_es_utils
{

using encode_handler_t = x26x_es_utils::encoder_handler_t<encoder_settings_t,
  encoding_session_t, session_params_t, sample_headers_t>;

} // x264_es_utils

#endif
