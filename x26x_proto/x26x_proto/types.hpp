/*
 * Copyright (C) 2026 CodeShop B.V.
 *
 * This file is part of the x26x service protocol library.
 *
 * The x26x service protocol library is free software: you can
 * redistribute it and/or modify it under the terms of version 2.1 of
 * the GNU Lesser General Public License as published by the Free
 * Software Foundation.
 *
 * The x26x service protocol library is distributed in the hope that
 * it will be useful, but WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See version 2.1 of the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of version 2.1 of the GNU Lesser
 * General Public License along with the x26x service protocol
 * library.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef X26X_PROTO_TYPES_HPP_
#define X26X_PROTO_TYPES_HPP_

#include "linkage.h"

#include <cuti/enum_mapping.hpp>
#include <cuti/tuple_mapping.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

namespace x26x_proto
{

enum class format_t
{
  NV12,
  YUV420P,
  YUV420P10LE,
};

X26X_PROTO_ABI std::string to_string(format_t format);

struct X26X_PROTO_ABI common_session_params_t
{
  common_session_params_t();

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

  bool operator==(common_session_params_t const& rhs) const = default;
};

struct X26X_PROTO_ABI frame_t
{
  frame_t();

  uint32_t width_;
  uint32_t height_;
  format_t format_;
  uint64_t pts_;
  uint32_t timescale_;
  bool keyframe_;
  std::vector<uint8_t> data_;

  bool operator==(frame_t const& rhs) const = default;
};

X26X_PROTO_ABI std::size_t frame_size(uint32_t width, uint32_t height,
  format_t format);

struct X26X_PROTO_ABI sample_t
{
  sample_t();

  int64_t dts_;
  int64_t pts_;
  enum class type_t { i, p, b, b_ref } type_;
  std::vector<uint8_t> data_;

  bool operator==(sample_t const& rhs) const = default;
};

X26X_PROTO_ABI std::string to_string(sample_t::type_t type);

} // x26x_proto

// adapters for cuti serialization

template<>
struct X26X_PROTO_ABI cuti::enum_mapping_t<x26x_proto::format_t>
{
  using underlying_t = std::underlying_type_t<x26x_proto::format_t>;

  static x26x_proto::format_t from_underlying(underlying_t value);
};

template<>
struct X26X_PROTO_ABI cuti::tuple_mapping_t<x26x_proto::common_session_params_t>
{
  using tuple_t = std::tuple<
    uint32_t,
    uint32_t,
    uint32_t,
    uint32_t,
    uint16_t,
    uint16_t,
    x26x_proto::format_t>;

  static tuple_t to_tuple(x26x_proto::common_session_params_t value);

  static x26x_proto::common_session_params_t from_tuple(tuple_t tuple);
};

template<>
struct X26X_PROTO_ABI cuti::tuple_mapping_t<x26x_proto::frame_t>
{
  using tuple_t = std::tuple<
    uint32_t,
    uint32_t,
    x26x_proto::format_t,
    uint64_t,
    uint32_t,
    bool,
    std::vector<uint8_t>>;

  static tuple_t to_tuple(x26x_proto::frame_t value);

  static x26x_proto::frame_t from_tuple(tuple_t tuple);
};

template<>
struct X26X_PROTO_ABI cuti::enum_mapping_t<x26x_proto::sample_t::type_t>
{
  using underlying_t = std::underlying_type_t<x26x_proto::sample_t::type_t>;

  static x26x_proto::sample_t::type_t from_underlying(underlying_t value);
};

template<>
struct X26X_PROTO_ABI cuti::tuple_mapping_t<x26x_proto::sample_t>
{
  using tuple_t = std::tuple<
    int64_t,
    int64_t,
    x26x_proto::sample_t::type_t,
    std::vector<uint8_t>>;

  static tuple_t to_tuple(x26x_proto::sample_t value);

  static x26x_proto::sample_t from_tuple(tuple_t tuple);
};

#endif
