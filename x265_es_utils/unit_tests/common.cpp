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

#include "common.hpp"

namespace common
{

x265_proto::session_params_t make_test_session_params(
  uint32_t timescale, uint32_t bitrate,
  uint32_t width, uint32_t height,
  x26x_proto::format_t format)
{
  x265_proto::session_params_t session_params;

  session_params.common_.timescale_ = timescale;
  session_params.common_.bitrate_ = bitrate;
  session_params.common_.width_ = width;
  session_params.common_.height_ = height;
  session_params.common_.format_ = format;
  session_params.general_profile_idc_ =
    format == x26x_proto::format_t::YUV420P10LE ?
      x265_proto::profile_t::MAIN10 : x265_proto::profile_t::MAIN;
  session_params.general_level_idc_ = 5 * 30;

  return session_params;
}

} // common
