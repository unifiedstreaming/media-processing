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

#ifndef X265_ES_UTILS_SERVICE_HPP_
#define X265_ES_UTILS_SERVICE_HPP_

#include "encoder_settings.hpp"
#include "encoding_session.hpp"

#include <x265_proto/types.hpp>
#include <x26x_es_utils/service.hpp>

namespace x265_es_utils
{

using service_t = x26x_es_utils::service_t<encoder_settings_t,
  encoding_session_t, x265_proto::session_params_t,
  x265_proto::sample_headers_t>;

} // x265_es_utils

#endif
