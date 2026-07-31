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

#include "common.hpp"

namespace common
{

x264_proto::session_params_t make_test_session_params(
  uint32_t timescale, uint32_t bitrate,
  uint32_t width, uint32_t height,
  x26x_proto::format_t format)
{
  x264_proto::session_params_t session_params;

  session_params.common_.timescale_ = timescale;
  session_params.common_.bitrate_ = bitrate;
  session_params.common_.width_ = width;
  session_params.common_.height_ = height;
  session_params.common_.format_ = format;
  session_params.profile_idc_ = format == x26x_proto::format_t::YUV420P10LE ?
    x264_proto::profile_t::HIGH10 : x264_proto::profile_t::MAIN;

  return session_params;
}

} // common
