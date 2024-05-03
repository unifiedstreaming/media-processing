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

#include <cuti/parse_error.hpp>
#include <cuti/tuple_mapping.hpp>

#include <optional>
#include <string>
#include <tuple>
#include <utility>

namespace x264_proto
{

enum class format_t : unsigned
{
  NV12,
};

enum class profile_t : unsigned
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
  session_params_t(
    unsigned timescale,
    unsigned bitrate,
    unsigned width,
    unsigned height,
    unsigned sar_width,
    unsigned sar_height,
    format_t format,
    profile_t profile_idc,
    unsigned level_idc,
    std::optional<bool> overscan_appropriate_flag,
    std::optional<unsigned> video_format,
    std::optional<bool> video_full_range_flag,
    std::optional<unsigned> colour_primaries,
    std::optional<unsigned> transfer_characteristics,
    std::optional<unsigned> matrix_coefficients,
    std::optional<unsigned> chroma_sample_loc_type_top_field,
    std::optional<unsigned> chroma_sample_loc_type_bottom_field,
    unsigned framerate_num,
    unsigned framerate_den);

  // MediaHeaderBox
  unsigned timescale_;

  // SampleEntry
  unsigned bitrate_;

  // VisualSampleEntry
  unsigned width_;
  unsigned height_;
  unsigned sar_width_;
  unsigned sar_height_;
  format_t format_;

  // AVCSampleEntry
  profile_t profile_idc_;
  unsigned level_idc_;

  // VUI parameters
  std::optional<bool> overscan_appropriate_flag_;
  std::optional<unsigned> video_format_;
  std::optional<bool> video_full_range_flag_;
  std::optional<unsigned> colour_primaries_;
  std::optional<unsigned> transfer_characteristics_;
  std::optional<unsigned> matrix_coefficients_;
  std::optional<unsigned> chroma_sample_loc_type_top_field_;
  std::optional<unsigned> chroma_sample_loc_type_bottom_field_;

  // Derived from VUI parameters
  unsigned framerate_num_;
  unsigned framerate_den_;

  bool operator==(session_params_t const& rhs) const;

  bool operator!=(session_params_t const& rhs) const
  { return !(*this == rhs); }
};

struct X264_PROTO_ABI frame_t
{
  // TBD

  bool operator==(frame_t const& rhs) const
  { return true; }

  bool operator!=(frame_t const& rhs) const
  { return !(*this == rhs); }
};

struct X264_PROTO_ABI sample_headers_t
{
  // TBD

  bool operator==(sample_headers_t const& rhs) const
  { return true; }

  bool operator!=(sample_headers_t const& rhs) const
  { return !(*this == rhs); }
};

struct X264_PROTO_ABI sample_t
{
  // TBD

  bool operator==(sample_t const& rhs) const
  { return true; }

  bool operator!=(sample_t const& rhs) const
  { return !(*this == rhs); }
};

} // x264_proto

// adapters for cuti serialization

template<>
struct X264_PROTO_ABI cuti::tuple_mapping_t<x264_proto::format_t>
{
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
    unsigned,
    unsigned,
    unsigned,
    unsigned,
    unsigned,
    unsigned,
    x264_proto::format_t,
    x264_proto::profile_t,
    unsigned,
    std::optional<bool>,
    std::optional<unsigned>,
    std::optional<bool>,
    std::optional<unsigned>,
    std::optional<unsigned>,
    std::optional<unsigned>,
    std::optional<unsigned>,
    std::optional<unsigned>,
    unsigned,
    unsigned>;

  static tuple_t to_tuple(x264_proto::session_params_t value);

  static x264_proto::session_params_t from_tuple(tuple_t tuple);
};

template<>
struct X264_PROTO_ABI cuti::tuple_mapping_t<x264_proto::frame_t>
{
  using tuple_t = std::tuple<>; // TBD

  static tuple_t to_tuple(x264_proto::frame_t value);

  static x264_proto::frame_t from_tuple(tuple_t tuple);
};

template<>
struct X264_PROTO_ABI cuti::tuple_mapping_t<x264_proto::sample_headers_t>
{
  using tuple_t = std::tuple<>; // TBD

  static tuple_t to_tuple(x264_proto::sample_headers_t value);

  static x264_proto::sample_headers_t from_tuple(tuple_t tuple);
};

template<>
struct X264_PROTO_ABI cuti::tuple_mapping_t<x264_proto::sample_t>
{
  using tuple_t = std::tuple<>; // TBD

  static tuple_t to_tuple(x264_proto::sample_t value);

  static x264_proto::sample_t from_tuple(tuple_t tuple);
};

#endif
