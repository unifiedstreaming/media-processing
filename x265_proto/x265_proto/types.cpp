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

#include "types.hpp"

#include <cuti/exception_builder.hpp>
#include <cuti/parse_error.hpp>

namespace x265_proto
{

std::string to_string(profile_t profile)
{
  switch(profile)
  {
  case profile_t::MAIN:
    return "MAIN";
  case profile_t::MAIN10:
    return "MAIN10";
  case profile_t::MAIN_STILL:
    return "MAIN_STILL";
  case profile_t::FORMAT_RANGE_EXTENSIONS:
    return "FORMAT_RANGE_EXTENSIONS";
  case profile_t::HIGH:
    return "HIGH";
  case profile_t::SCREEN_CONTENT:
    return "SCREEN_CONTENT";
  default:
    return "unknown x265_proto::profile_t value " +
      std::to_string(cuti::to_underlying(profile));
  }
}

session_params_t::session_params_t()
: common_()
, general_tier_flag_(false)
, general_profile_idc_(profile_t::MAIN)
, general_level_idc_(30)
, vui_overscan_appropriate_flag_(std::nullopt)
, vui_video_format_(std::nullopt)
, vui_video_full_range_flag_(std::nullopt)
, vui_colour_primaries_(std::nullopt)
, vui_transfer_characteristics_(std::nullopt)
, vui_matrix_coefficients_(std::nullopt)
, vui_chroma_sample_loc_type_top_field_(std::nullopt)
, vui_chroma_sample_loc_type_bottom_field_(std::nullopt)
, vui_def_disp_win_left_offset_(std::nullopt)
, vui_def_disp_win_right_offset_(std::nullopt)
, vui_def_disp_win_top_offset_(std::nullopt)
, vui_def_disp_win_bottom_offset_(std::nullopt)
{
}

sample_headers_t::sample_headers_t()
: vps_()
, sps_()
, pps_()
{
}

} // x265_proto

x265_proto::profile_t
cuti::enum_mapping_t<x265_proto::profile_t>::from_underlying(
  underlying_t value)
{
  switch(value)
  {
  case to_underlying(x265_proto::profile_t::MAIN):
  case to_underlying(x265_proto::profile_t::MAIN10):
  case to_underlying(x265_proto::profile_t::MAIN_STILL):
  case to_underlying(x265_proto::profile_t::FORMAT_RANGE_EXTENSIONS):
  case to_underlying(x265_proto::profile_t::HIGH):
  case to_underlying(x265_proto::profile_t::SCREEN_CONTENT):
    return x265_proto::profile_t{value};
  default:
    exception_builder_t<parse_error_t> builder;
    builder << "bad x265_proto::profile_t value " << to_serialized(value);
    builder.explode();
  }
}

cuti::tuple_mapping_t<x265_proto::session_params_t>::tuple_t
cuti::tuple_mapping_t<x265_proto::session_params_t>::to_tuple(
  x265_proto::session_params_t value)
{
  return tuple_t(
    value.common_,
    value.general_tier_flag_,
    value.general_profile_idc_,
    value.general_level_idc_,
    value.vui_overscan_appropriate_flag_,
    value.vui_video_format_,
    value.vui_video_full_range_flag_,
    value.vui_colour_primaries_,
    value.vui_transfer_characteristics_,
    value.vui_matrix_coefficients_,
    value.vui_chroma_sample_loc_type_top_field_,
    value.vui_chroma_sample_loc_type_bottom_field_,
    value.vui_def_disp_win_left_offset_,
    value.vui_def_disp_win_right_offset_,
    value.vui_def_disp_win_top_offset_,
    value.vui_def_disp_win_bottom_offset_);
}

x265_proto::session_params_t
cuti::tuple_mapping_t<x265_proto::session_params_t>::from_tuple(tuple_t tuple)
{
  x265_proto::session_params_t value;
  value.common_ = std::get<0>(tuple);
  value.general_tier_flag_ = std::get<1>(tuple);
  value.general_profile_idc_ = std::get<2>(tuple);
  value.general_level_idc_ = std::get<3>(tuple);
  value.vui_overscan_appropriate_flag_ = std::get<4>(tuple);
  value.vui_video_format_ = std::get<5>(tuple);
  value.vui_video_full_range_flag_ = std::get<6>(tuple);
  value.vui_colour_primaries_ = std::get<7>(tuple);
  value.vui_transfer_characteristics_ = std::get<8>(tuple);
  value.vui_matrix_coefficients_ = std::get<9>(tuple);
  value.vui_chroma_sample_loc_type_top_field_ = std::get<10>(tuple);
  value.vui_chroma_sample_loc_type_bottom_field_ = std::get<11>(tuple);
  value.vui_def_disp_win_left_offset_ = std::get<12>(tuple);
  value.vui_def_disp_win_right_offset_ = std::get<13>(tuple);
  value.vui_def_disp_win_top_offset_ = std::get<14>(tuple);
  value.vui_def_disp_win_bottom_offset_ = std::get<15>(tuple);
  return value;
}

cuti::tuple_mapping_t<x265_proto::sample_headers_t>::tuple_t
cuti::tuple_mapping_t<x265_proto::sample_headers_t>::to_tuple(
  x265_proto::sample_headers_t value)
{
  return tuple_t(
    std::move(value.vps_),
    std::move(value.sps_),
    std::move(value.pps_));
}

x265_proto::sample_headers_t
cuti::tuple_mapping_t<x265_proto::sample_headers_t>::from_tuple(tuple_t tuple)
{
  x265_proto::sample_headers_t value;
  value.vps_ = std::move(std::get<0>(tuple));
  value.sps_ = std::move(std::get<1>(tuple));
  value.pps_ = std::move(std::get<2>(tuple));
  return value;
}
