/*
 * Copyright (C) 2024 CodeShop B.V.
 *
 * This file is part of the x264 service protocol library.
 *
 * The x264 service protocol library is free software: you can
 * redistribute it and/or modify it under the terms of version 2.1 of
 * the GNU Lesser General Public License as published by the Free
 * Software Foundation.
 *
 * The x264 service protocol library is distributed in the hope that
 * it will be useful, but WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See version 2.1 of the GNU Lesser General Public License
 * for more details.
 * 
 * You should have received a copy of version 2.1 of the GNU Lesser
 * General Public License along with the x264 service protocol
 * library.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef X264_PROTO_TYPES_HPP_
#define X264_PROTO_TYPES_HPP_

#include "linkage.h"

#include <cuti/tuple_mapping.hpp>

#include <cstdint>
#include <optional>
#include <vector>

namespace x264_proto
{

enum class format_t
{
  NV12,
};

enum class profile_t
{
  BASELINE = 66,
  MAIN = 77,
  HIGH = 100,
  HIGH10 = 110,
  HIGH422 = 122,
  HIGH444_PREDICTIVE = 244,
};

struct X264_PROTO_ABI session_params_t
{
  session_params_t();

  // MediaHeaderBox
  uint32_t timescale_;

  // SampleEntry
  uint32_t bitrate_;

  // VisualSampleEntry
  uint32_t width_;
  uint32_t height_;
  uint16_t sar_width_;
  uint16_t sar_height_;
  format_t format_;

  // AVCSampleEntry
  profile_t profile_idc_;
  uint16_t level_idc_;

  // VUI parameters
  std::optional<bool> overscan_appropriate_flag_;
  std::optional<uint16_t> video_format_;
  std::optional<bool> video_full_range_flag_;
  std::optional<uint16_t> colour_primaries_;
  std::optional<uint16_t> transfer_characteristics_;
  std::optional<uint16_t> matrix_coefficients_;
  std::optional<uint32_t> chroma_sample_loc_type_top_field_;
  std::optional<uint32_t> chroma_sample_loc_type_bottom_field_;

  // Derived from VUI parameters
  uint32_t framerate_num_;
  uint32_t framerate_den_;

  bool operator==(session_params_t const& rhs) const;

  bool operator!=(session_params_t const& rhs) const
  { return !(*this == rhs); }
};

struct X264_PROTO_ABI frame_t
{
  frame_t();

  uint32_t width_;
  uint32_t height_;
  format_t format_;
  uint64_t pts_;
  uint32_t timescale_;
  bool keyframe_;
  std::vector<uint8_t> data_;

  bool operator==(frame_t const& rhs) const;

  bool operator!=(frame_t const& rhs) const
  { return !(*this == rhs); }
};

struct X264_PROTO_ABI sample_headers_t
{
  sample_headers_t();

  std::vector<uint8_t> sps_;
  std::vector<uint8_t> pps_;

  bool operator==(sample_headers_t const& rhs) const;

  bool operator!=(sample_headers_t const& rhs) const
  { return !(*this == rhs); }
};

struct X264_PROTO_ABI sample_t
{
  sample_t();

  int64_t dts_;
  int64_t pts_;
  enum class type_t { i, p, b, b_ref } type_;
  std::vector<uint8_t> data_;

  bool operator==(sample_t const& rhs) const;

  bool operator!=(sample_t const& rhs) const
  { return !(*this == rhs); }
};

} // x264_proto

// adapters for cuti serialization

template<>
struct X264_PROTO_ABI cuti::tuple_mapping_t<x264_proto::format_t>
{
  // TBD: migrate to serialized_enum_t
  using underlying_t = std::underlying_type_t<x264_proto::format_t>;
  using tuple_t = std::tuple<underlying_t>;

  static tuple_t to_tuple(x264_proto::format_t value)
  {
    return {static_cast<underlying_t>(value)};
  }

  static x264_proto::format_t from_tuple(tuple_t tuple);
};

template<>
struct X264_PROTO_ABI cuti::tuple_mapping_t<x264_proto::profile_t>
{
  // TBD: migrate to serialized_enum_t
  using underlying_t = std::underlying_type_t<x264_proto::profile_t>;
  using tuple_t = std::tuple<underlying_t>;

  static tuple_t to_tuple(x264_proto::profile_t value)
  {
    return {static_cast<underlying_t>(value)};
  }

  static x264_proto::profile_t from_tuple(tuple_t tuple);
};

template<>
struct X264_PROTO_ABI cuti::tuple_mapping_t<x264_proto::session_params_t>
{
  using tuple_t = std::tuple<
    uint32_t,
    uint32_t,
    uint32_t,
    uint32_t,
    uint16_t,
    uint16_t,
    x264_proto::format_t,
    x264_proto::profile_t,
    uint16_t,
    std::optional<bool>,
    std::optional<uint16_t>,
    std::optional<bool>,
    std::optional<uint16_t>,
    std::optional<uint16_t>,
    std::optional<uint16_t>,
    std::optional<uint32_t>,
    std::optional<uint32_t>,
    uint32_t,
    uint32_t>;

  static tuple_t to_tuple(x264_proto::session_params_t value);

  static x264_proto::session_params_t from_tuple(tuple_t tuple);
};

template<>
struct X264_PROTO_ABI cuti::tuple_mapping_t<x264_proto::frame_t>
{
  using tuple_t = std::tuple<
    uint32_t,
    uint32_t,
    x264_proto::format_t,
    uint64_t,
    uint32_t,
    bool,
    std::vector<uint8_t>>;

  static tuple_t to_tuple(x264_proto::frame_t value);

  static x264_proto::frame_t from_tuple(tuple_t tuple);
};

template<>
struct X264_PROTO_ABI cuti::tuple_mapping_t<x264_proto::sample_headers_t>
{
  using tuple_t = std::tuple<
    std::vector<uint8_t>,
    std::vector<uint8_t>>;

  static tuple_t to_tuple(x264_proto::sample_headers_t value);

  static x264_proto::sample_headers_t from_tuple(tuple_t tuple);
};

template<>
struct X264_PROTO_ABI cuti::tuple_mapping_t<x264_proto::sample_t::type_t>
{
  // TBD: migrate to serialized_enum_t
  using underlying_t = std::underlying_type_t<x264_proto::sample_t::type_t>;
  using tuple_t = std::tuple<underlying_t>;

  static tuple_t to_tuple(x264_proto::sample_t::type_t value)
  {
    return {static_cast<underlying_t>(value)};
  }

  static x264_proto::sample_t::type_t from_tuple(tuple_t tuple);
};

template<>
struct X264_PROTO_ABI cuti::tuple_mapping_t<x264_proto::sample_t>
{
  using tuple_t = std::tuple<
    int64_t,
    int64_t,
    x264_proto::sample_t::type_t,
    std::vector<uint8_t>>;

  static tuple_t to_tuple(x264_proto::sample_t value);

  static x264_proto::sample_t from_tuple(tuple_t tuple);
};

#endif
