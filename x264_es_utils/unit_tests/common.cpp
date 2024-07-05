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
#include <limits>

namespace common
{

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
  session_params.profile_idc_ = format == x264_proto::format_t::YUV420P10LE ?
    x264_proto::profile_t::HIGH10 : x264_proto::profile_t::MAIN;

  return session_params;
}

namespace // anonymous
{

std::vector<uint8_t> make_test_frame_data_nv12(
  uint32_t width, uint32_t height,
  uint8_t y, uint8_t u, uint8_t v)
{
  assert(width % 2 == 0 && height % 2 == 0);
  std::size_t const num_y = width * height;
  std::size_t const num_uv = (width / 2) * (height / 2);
  std::size_t const size = (num_y + num_uv * 2) * sizeof(uint8_t);

  std::vector<uint8_t> data;
  data.reserve(size);

  data.insert(data.end(), num_y, y);
  if(u == v)
  {
    data.insert(data.end(), num_uv * 2, u);
  }
  else
  {
    for(std::size_t i = 0; i < num_uv; ++i)
    {
      data.insert(data.end(), u);
      data.insert(data.end(), v);
    }
  }

  assert(data.size() == size);
  return data;
}

std::vector<uint8_t> make_test_frame_data_yuv420p(
  uint32_t width, uint32_t height,
  uint8_t y, uint8_t u, uint8_t v)
{
  assert(width % 2 == 0 && height % 2 == 0);
  std::size_t const num_y = width * height;
  std::size_t const num_uv = (width / 2) * (height / 2);
  std::size_t const size = (num_y + num_uv * 2) * sizeof(uint8_t);

  std::vector<uint8_t> data;
  data.reserve(size);

  data.insert(data.end(), num_y, y);
  if(u == v)
  {
    data.insert(data.end(), num_uv * 2, u);
  }
  else
  {
    data.insert(data.end(), num_uv, u);
    data.insert(data.end(), num_uv, v);
  }

  assert(data.size() == size);
  return data;
}

std::pair<uint8_t, uint8_t> split(uint16_t v)
{
  return {v & 0xff, v >> 8};
}

std::vector<uint8_t> make_test_frame_data_yuv420p10le(
  uint32_t width, uint32_t height,
  uint16_t y, uint16_t u, uint16_t v)
{
  assert(width % 2 == 0 && height % 2 == 0);
  std::size_t const num_y = width * height;
  std::size_t const num_uv = (width / 2) * (height / 2);
  std::size_t const size = (num_y + num_uv * 2) * sizeof(uint16_t);

  std::vector<uint8_t> data;
  data.reserve(size);

  {
    auto [y_lo, y_hi] = split(y);
    for(std::size_t i = 0; i < num_y; ++i)
    {
      data.push_back(y_lo);
      data.push_back(y_hi);
    }
  }

  {
    auto [u_lo, u_hi] = split(u);
    for(std::size_t i = 0; i < num_uv; ++i)
    {
      data.push_back(u_lo);
      data.push_back(u_hi);
    }
  }

  {
    auto [v_lo, v_hi] = split(v);
    for(std::size_t i = 0; i < num_uv; ++i)
    {
      data.push_back(v_lo);
      data.push_back(v_hi);
    }
  }

  assert(data.size() == size);
  return data;
}

uint8_t to_8bit(component_t component)
{
  assert(component <= std::numeric_limits<uint8_t>::max());
  return static_cast<uint8_t>(component);
}

} // anonymous

std::vector<uint8_t> make_test_frame_data(
  uint32_t width, uint32_t height,
  x264_proto::format_t format, yuv_t yuv)
{
  switch(format)
  {
  case x264_proto::format_t::NV12:
    return make_test_frame_data_nv12(width, height,
      to_8bit(yuv.y_), to_8bit(yuv.u_), to_8bit(yuv.v_));
  case x264_proto::format_t::YUV420P:
    return make_test_frame_data_yuv420p(width, height,
      to_8bit(yuv.y_), to_8bit(yuv.u_), to_8bit(yuv.v_));
  case x264_proto::format_t::YUV420P10LE:
    return make_test_frame_data_yuv420p10le(width, height,
      yuv.y_, yuv.u_, yuv.v_);
  default:
    cuti::system_exception_builder_t builder;
    builder << "bad x264_proto::format_t value " <<
      std::to_string(cuti::to_underlying(format));
    builder.explode();
  }
}

x264_proto::frame_t make_test_frame(
  uint32_t width, uint32_t height,
  x264_proto::format_t format,
  uint64_t pts, uint32_t timescale,
  bool keyframe,
  std::vector<uint8_t>&& data)
{
  x264_proto::frame_t frame;

  frame.width_ = width;
  frame.height_ = height;
  frame.format_ = format;
  frame.pts_ = pts;
  frame.timescale_ = timescale;
  frame.keyframe_ = keyframe;
  frame.data_ = std::move(data);

  return frame;
}

x264_proto::frame_t make_test_frame(
  uint32_t width, uint32_t height,
  x264_proto::format_t format,
  uint64_t pts, uint32_t timescale,
  bool keyframe,
  yuv_t yuv)
{
  return make_test_frame(width, height, format, pts, timescale, keyframe,
    make_test_frame_data(width, height, format, yuv));
}

std::vector<x264_proto::frame_t> make_test_frames(
  std::size_t count, std::size_t gop_size,
  uint32_t width, uint32_t height,
  x264_proto::format_t format,
  uint32_t timescale, uint32_t duration,
  yuv_t yuv)
{
  std::vector<x264_proto::frame_t> frames;

  uint64_t pts = 0;
  for(std::size_t i = 0; i < count; ++i, pts += duration)
  {
    bool keyframe = i % gop_size == 0;
    frames.push_back(
      make_test_frame(width, height, format, pts, timescale, keyframe, yuv));
  }

  return frames;
}

namespace // anonymous
{

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
  std::size_t count, std::size_t gop_size,
  uint32_t width, uint32_t height,
  x264_proto::format_t format,
  uint32_t timescale, uint32_t duration)
{
  std::vector<x264_proto::frame_t> frames;

  uint64_t pts = 0;
  double hue = 0;
  const double hue_inc = 1.0 / count;
  const double sat = 1.0;
  const double val = 1.0;

  for(std::size_t i = 0; i < count; ++i, pts += duration, hue += hue_inc)
  {
    bool keyframe = i % gop_size == 0;
    auto yuv = hsv2yuv(hue, sat, val);
    frames.push_back(
      make_test_frame(width, height, format, pts, timescale, keyframe, yuv));
  }

  return frames;
}

std::vector<x264_proto::frame_t> make_test_frames_from_file(
  std::string filename, std::size_t gop_size,
  uint32_t width, uint32_t height,
  x264_proto::format_t format,
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
  std::size_t frame_size = x264_proto::frame_size(width, height, format);

  uint64_t pts = 0;
  for(std::size_t i = 0; ; ++i, pts += duration)
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
    if(count <= 0)
    {
      break;
    }
    else if(static_cast<std::size_t>(count) != frame_size)
    {
      cuti::system_exception_builder_t builder;
      builder << "could only read " << count << " bytes from " << filename <<
        ", expected to read " << frame_size;
      builder.explode();
    }
    frames.push_back(make_test_frame(width, height, format, pts, timescale,
      keyframe, std::move(data)));
  }

  return frames;
}

} // common
