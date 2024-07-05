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

#ifndef X264_ES_UTILS_UNIT_TESTS_COMMON_HPP_
#define X264_ES_UTILS_UNIT_TESTS_COMMON_HPP_

#include <x264_proto/types.hpp>

namespace fnv1a32
{

// FNV-1a offset basis
constexpr uint32_t init = 2166136261;

// 32 bit magic FNV-1 prime (seed<<1)+(seed<<4)+(seed<<7)+(seed<<8)+(seed<<24)
constexpr uint32_t prime = 16777619;

struct hash_t
{
  hash_t()
  : seed_(init)
  {
  }

  hash_t(hash_t const&) = delete;
  hash_t& operator=(hash_t const&) = delete;

  void update(uint8_t const* first, uint8_t const* last)
  {
    while(first != last)
    {
      seed_ ^= static_cast<uint32_t>(*first++);
      seed_ *= prime;
    }
  }

  uint32_t final() const
  {
    return seed_;
  }

private:
  uint32_t seed_;
};

inline uint32_t hash(uint8_t const* first, size_t size)
{
  hash_t hash;
  hash.update(first, first + size);
  return hash.final();
}

} // fnv1a32

using component_t = uint16_t;

constexpr component_t black_y_8 = 0x10; // (1 << 8) / 16
constexpr component_t black_u_8 = 0x80; // (1 << 8) / 2
constexpr component_t black_v_8 = 0x80; // (1 << 8) / 2

constexpr component_t black_y_10 = 0x0040; // (1 << 10) / 16
constexpr component_t black_u_10 = 0x0200; // (1 << 10) / 2
constexpr component_t black_v_10 = 0x0200; // (1 << 10) / 2

x264_proto::session_params_t make_test_session_params(
  uint32_t timescale, uint32_t bitrate,
  uint32_t width, uint32_t height,
  x264_proto::format_t format);

std::vector<uint8_t> make_test_frame_data(
  uint32_t width, uint32_t height,
  x264_proto::format_t format,
  component_t y, component_t u, component_t v);

x264_proto::frame_t make_test_frame(
  uint32_t width, uint32_t height,
  x264_proto::format_t format,
  uint64_t pts, uint32_t timescale,
  bool keyframe,
  component_t y, component_t u, component_t v);

std::vector<x264_proto::frame_t> make_test_frames(
  size_t count, size_t gop_size,
  uint32_t width, uint32_t height,
  x264_proto::format_t format,
  uint32_t timescale, uint32_t duration,
  component_t y, component_t u, component_t v);

std::vector<x264_proto::frame_t> make_test_rainbow_frames(
  size_t count, size_t gop_size,
  uint32_t width, uint32_t height,
  x264_proto::format_t format,
  uint32_t timescale, uint32_t duration);

std::vector<x264_proto::frame_t> make_test_frames_from_file(
  std::string filename, size_t gop_size,
  uint32_t width, uint32_t height,
  x264_proto::format_t format,
  uint32_t timescale, uint32_t duration);

#endif //  X264_ES_UTILS_UNIT_TESTS_COMMON_HPP_
