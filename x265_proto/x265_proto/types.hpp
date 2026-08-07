/*
 * Copyright (C) 2026 CodeShop B.V.
 *
 * This file is part of the x265 service protocol library.
 *
 * The x265 service protocol library is free software: you can
 * redistribute it and/or modify it under the terms of version 2.1 of
 * the GNU Lesser General Public License as published by the Free
 * Software Foundation.
 *
 * The x265 service protocol library is distributed in the hope that
 * it will be useful, but WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See version 2.1 of the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of version 2.1 of the GNU Lesser
 * General Public License along with the x265 service protocol
 * library.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef X265_PROTO_TYPES_HPP_
#define X265_PROTO_TYPES_HPP_

#include "linkage.h"

#include <cuti/enum_mapping.hpp>
#include <cuti/tuple_mapping.hpp>

#include <x26x_proto/types.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

namespace x265_proto
{

enum class profile_t
{
  MAIN = 1,
  MAIN10 = 2,
  MAIN_STILL = 3,
  FORMAT_RANGE_EXTENSIONS = 4,
  HIGH = 5,
  SCREEN_CONTENT = 9
};

X265_PROTO_ABI std::string to_string(profile_t profile);

struct X265_PROTO_ABI session_params_t
{
  session_params_t();

  x26x_proto::common_session_params_t common_;

  // HEVCSampleEntry
  bool general_tier_flag_;
  profile_t general_profile_idc_;
  uint16_t general_level_idc_;

  // VUI parameters
  std::optional<bool> vui_overscan_appropriate_flag_;
  std::optional<uint16_t> vui_video_format_;
  std::optional<bool> vui_video_full_range_flag_;
  std::optional<uint16_t> vui_colour_primaries_;
  std::optional<uint16_t> vui_transfer_characteristics_;
  std::optional<uint16_t> vui_matrix_coefficients_;
  std::optional<uint32_t> vui_chroma_sample_loc_type_top_field_;
  std::optional<uint32_t> vui_chroma_sample_loc_type_bottom_field_;
  std::optional<uint32_t> vui_def_disp_win_left_offset_;
  std::optional<uint32_t> vui_def_disp_win_right_offset_;
  std::optional<uint32_t> vui_def_disp_win_top_offset_;
  std::optional<uint32_t> vui_def_disp_win_bottom_offset_;
  std::optional<uint32_t> vui_num_units_in_tick_;
  std::optional<uint32_t> vui_time_scale_;

  bool operator==(session_params_t const& rhs) const = default;
};

struct X265_PROTO_ABI sample_headers_t
{
  sample_headers_t();

  std::vector<uint8_t> vps_;
  std::vector<uint8_t> sps_;
  std::vector<uint8_t> pps_;

  bool operator==(sample_headers_t const& rhs) const = default;
};

} // x265_proto

// adapters for cuti serialization

template<>
struct X265_PROTO_ABI cuti::enum_mapping_t<x265_proto::profile_t>
{
  using underlying_t = std::underlying_type_t<x265_proto::profile_t>;

  static x265_proto::profile_t from_underlying(underlying_t value);
};

template<>
struct X265_PROTO_ABI cuti::tuple_mapping_t<x265_proto::session_params_t>
{
  using tuple_t = std::tuple<
    x26x_proto::common_session_params_t,
    bool,
    x265_proto::profile_t,
    uint16_t,
    std::optional<bool>,
    std::optional<uint16_t>,
    std::optional<bool>,
    std::optional<uint16_t>,
    std::optional<uint16_t>,
    std::optional<uint16_t>,
    std::optional<uint32_t>,
    std::optional<uint32_t>,
    std::optional<uint32_t>,
    std::optional<uint32_t>,
    std::optional<uint32_t>,
    std::optional<uint32_t>,
    std::optional<uint32_t>,
    std::optional<uint32_t>>;

  static tuple_t to_tuple(x265_proto::session_params_t value);

  static x265_proto::session_params_t from_tuple(tuple_t tuple);
};

template<>
struct X265_PROTO_ABI cuti::tuple_mapping_t<x265_proto::sample_headers_t>
{
  using tuple_t = std::tuple<
    std::vector<uint8_t>,
    std::vector<uint8_t>,
    std::vector<uint8_t>>;

  static tuple_t to_tuple(x265_proto::sample_headers_t value);

  static x265_proto::sample_headers_t from_tuple(tuple_t tuple);
};

#endif
