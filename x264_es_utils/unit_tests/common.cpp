/*
 * Copyright (C) 2024 CodeShop B.V.
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

x264_proto::session_params_t make_test_session_params(
  uint32_t timescale, uint32_t bitrate,
  uint32_t width, uint32_t height)
{
  x264_proto::session_params_t session_params;

  session_params.timescale_ = timescale;
  session_params.bitrate_ = bitrate;
  session_params.width_ = width;
  session_params.height_ = height;

  return session_params;
}

std::vector<uint8_t> make_test_frame_data(
  uint32_t width, uint32_t height,
  uint8_t y, uint8_t u, uint8_t v)
{
  std::vector<uint8_t> data;

  uint32_t size_y = width * height;
  uint32_t size_uv = width * height / 2;

  data.insert(data.end(), size_y, y);
  if(u == v)
  {
    data.insert(data.end(), size_uv, u);
  }
  else
  {
    for(uint32_t i = 0; i < size_uv; i +=2)
    {
      data.insert(data.end(), u);
      data.insert(data.end(), v);
    }
  }

  return data;
}

x264_proto::frame_t make_test_frame(
  uint32_t width, uint32_t height,
  uint64_t pts, uint32_t timescale,
  bool keyframe,
  uint8_t y, uint8_t u, uint8_t v)
{
  x264_proto::frame_t frame;

  frame.width_ = width;
  frame.height_ = height;
  frame.pts_ = pts;
  frame.timescale_ = timescale;
  frame.keyframe_ = keyframe;
  frame.data_ = make_test_frame_data(width, height, y, u, v);

  return frame;
}

std::vector<x264_proto::frame_t> make_test_frames(
  size_t count, size_t gop_size,
  uint32_t width, uint32_t height,
  uint32_t timescale, uint32_t duration,
  uint8_t y, uint8_t u, uint8_t v)
{
  std::vector<x264_proto::frame_t> frames;

  uint64_t pts = 0;
  for(size_t i = 0; i < count; ++i, pts += duration)
  {
    bool keyframe = i % gop_size == 0;
    frames.push_back(
      make_test_frame(width, height, pts, timescale, keyframe, y, u, v));
  }

  return frames;
}
