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

#include <cuti/system_error.hpp>

#undef NDEBUG
#include <cassert>
#include <fstream>

x264_proto::session_params_t make_test_session_params(
  uint32_t timescale, uint32_t bitrate,
  uint32_t width, uint32_t height,
  x264_proto::format_t format)
{
  x264_proto::session_params_t session_params;

  session_params.timescale_ = timescale;
  session_params.bitrate_ = bitrate;
  session_params.width_ = width;
  session_params.height_ = height;
  session_params.format_ = format;

  return session_params;
}

std::vector<uint8_t> make_test_frame_data(
  uint32_t width, uint32_t height,
  uint8_t y, uint8_t u, uint8_t v)
{
  std::vector<uint8_t> data;

  uint32_t const size_y = width * height;
  uint32_t const size_uv = width * height / 2;

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
  std::vector<uint8_t>&& data)
{
  x264_proto::frame_t frame;

  frame.width_ = width;
  frame.height_ = height;
  frame.pts_ = pts;
  frame.timescale_ = timescale;
  frame.keyframe_ = keyframe;
  frame.data_ = std::move(data);

  return frame;
}

x264_proto::frame_t make_test_frame(
  uint32_t width, uint32_t height,
  uint64_t pts, uint32_t timescale,
  bool keyframe,
  uint8_t y, uint8_t u, uint8_t v)
{
  return make_test_frame(width, height, pts, timescale, keyframe,
    make_test_frame_data(width, height, y, u, v));
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

namespace // anonymous
{

using yuv_t = std::tuple<uint8_t, uint8_t, uint8_t>;
using rgb_t = std::tuple<uint8_t, uint8_t, uint8_t>;

constexpr uint8_t round(double d)
{
  return static_cast<uint8_t>(d + 0.5);
}

constexpr yuv_t rgb2yuv_bt601(rgb_t rgb)
{
  auto const& [r, g, b] = rgb;
  return {
    round( 0.257 * r + 0.504 * g + 0.098 * b + 16 ),
    round(-0.148 * r - 0.291 * g + 0.439 * b + 128),
    round( 0.439 * r - 0.368 * g - 0.071 * b + 128)
  };
}

static_assert(rgb2yuv_bt601({0x00, 0x00, 0x00}) == yuv_t{0x10, 0x80, 0x80});
static_assert(rgb2yuv_bt601({0x80, 0x80, 0x80}) == yuv_t{0x7e, 0x80, 0x80});
static_assert(rgb2yuv_bt601({0xff, 0xff, 0xff}) == yuv_t{0xeb, 0x80, 0x80});

static_assert(rgb2yuv_bt601({0x80, 0x00, 0x00}) == yuv_t{0x31, 0x6d, 0xb8});
static_assert(rgb2yuv_bt601({0x00, 0x80, 0x00}) == yuv_t{0x51, 0x5b, 0x51});
static_assert(rgb2yuv_bt601({0x00, 0x00, 0x80}) == yuv_t{0x1d, 0xb8, 0x77});

rgb_t hsv2rgb(double h, double s, double v)
{
  assert(h >= 0.0 && h <= 1.0);
  assert(s >= 0.0 && s <= 1.0);
  assert(v >= 0.0 && v <= 1.0);

  double h6 = h * 6;
  long i = static_cast<long>(h6);
  double f = h6 - i;
  double p = v * (1 - s);
  double q = v * (1 - f * s);
  double t = v * (1 - (1 - f) * s);

  double r, g, b;
  switch(i % 6)
  {
  case 0:  r = v; g = t; b = p; break;
  case 1:  r = q; g = v; b = p; break;
  case 2:  r = p; g = v; b = t; break;
  case 3:  r = p; g = q; b = v; break;
  case 4:  r = t; g = p; b = v; break;
  default: r = v; g = p; b = q; break;
  }

  assert(r >= 0.0 && r <= 1.0);
  assert(g >= 0.0 && g <= 1.0);
  assert(b >= 0.0 && b <= 1.0);

  return {round(r * 255), round(g * 255), round(b * 255)};
}

yuv_t hsv2yuv(double h, double s, double v)
{
  return rgb2yuv_bt601(hsv2rgb(h, s, v));
}

} // anonymous

std::vector<x264_proto::frame_t> make_test_rainbow_frames(
  size_t count, size_t gop_size,
  uint32_t width, uint32_t height,
  uint32_t timescale, uint32_t duration)
{
  std::vector<x264_proto::frame_t> frames;

  uint64_t pts = 0;
  double hue = 0;
  const double hue_inc = 1.0 / count;
  const double sat = 1.0;
  const double val = 1.0;

  for(size_t i = 0; i < count; ++i, pts += duration, hue += hue_inc)
  {
    bool keyframe = i % gop_size == 0;
    auto [y, u, v] = hsv2yuv(hue, sat, val);
    frames.push_back(
      make_test_frame(width, height, pts, timescale, keyframe, y, u, v));
  }

  return frames;
}

std::vector<x264_proto::frame_t> make_test_frames_from_file(
  std::string filename, size_t gop_size,
  uint32_t width, uint32_t height,
  uint32_t timescale, uint32_t duration)
{
  std::ifstream ifs(filename, std::ios::binary);
  if(ifs.fail())
  {
    cuti::system_exception_builder_t builder;
    builder << "cannot open file " << filename;
    builder.explode();
  }

  std::vector<x264_proto::frame_t> frames;
  uint32_t const frame_size = width * height * 3 / 2;

  uint64_t pts = 0;
  for(size_t i = 0; ; ++i, pts += duration)
  {
    bool keyframe = i % gop_size == 0;
    std::vector<uint8_t> data(frame_size);
    ifs.read(reinterpret_cast<char*>(data.data()), frame_size);
    if(ifs.bad())
    {
      cuti::system_exception_builder_t builder;
      builder << "cannot read file " << filename;
      builder.explode();
    }
    auto count = ifs.gcount();
    if(count == 0)
    {
      break;
    }
    else if(count != frame_size)
    {
      cuti::system_exception_builder_t builder;
      builder << "could only read " << count << " bytes from " << filename <<
        ", expected to read " << frame_size;
      builder.explode();
    }
    frames.push_back(make_test_frame(width, height, pts, timescale, keyframe,
      std::move(data)));
  }

  return frames;
}
