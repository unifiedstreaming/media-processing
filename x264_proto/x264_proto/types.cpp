/*
 * Copyright (C) 2024-2026 CodeShop B.V.
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

#include "types.hpp"

#include <cuti/exception_builder.hpp>
#include <cuti/parse_error.hpp>

namespace x264_proto
{

std::string to_string(profile_t profile)
{
  switch(profile)
  {
  case profile_t::BASELINE:
    return "BASELINE";
  case profile_t::MAIN:
    return "MAIN";
  case profile_t::HIGH:
    return "HIGH";
  case profile_t::HIGH10:
    return "HIGH10";
  case profile_t::HIGH422:
    return "HIGH422";
  case profile_t::HIGH444_PREDICTIVE:
    return "HIGH444_PREDICTIVE";
  default:
    return "unknown x264_proto::profile_t value " +
      std::to_string(cuti::to_underlying(profile));
  }
}

session_params_t::session_params_t()
: common_()
, profile_idc_(profile_t::BASELINE)
, level_idc_(30)
, vui_overscan_appropriate_flag_(std::nullopt)
, vui_video_format_(std::nullopt)
, vui_video_full_range_flag_(std::nullopt)
, vui_colour_primaries_(std::nullopt)
, vui_transfer_characteristics_(std::nullopt)
, vui_matrix_coefficients_(std::nullopt)
, vui_chroma_sample_loc_type_top_field_(std::nullopt)
, vui_chroma_sample_loc_type_bottom_field_(std::nullopt)
, vui_num_units_in_tick_(std::nullopt)
, vui_time_scale_(std::nullopt)
, vui_fixed_frame_rate_flag_(std::nullopt)
{
}

sample_headers_t::sample_headers_t()
: sps_()
, pps_()
{
}

} // x264_proto

x264_proto::profile_t
cuti::enum_mapping_t<x264_proto::profile_t>::from_underlying(
  underlying_t value)
{
  switch(value)
  {
  case to_underlying(x264_proto::profile_t::BASELINE):
  case to_underlying(x264_proto::profile_t::MAIN):
  case to_underlying(x264_proto::profile_t::HIGH):
  case to_underlying(x264_proto::profile_t::HIGH10):
  case to_underlying(x264_proto::profile_t::HIGH422):
  case to_underlying(x264_proto::profile_t::HIGH444_PREDICTIVE):
    return x264_proto::profile_t{value};
  default:
    exception_builder_t<parse_error_t> builder;
    builder << "bad x264_proto::profile_t value " << to_serialized(value);
    builder.explode();
  }
}

cuti::tuple_mapping_t<x264_proto::session_params_t>::tuple_t
cuti::tuple_mapping_t<x264_proto::session_params_t>::to_tuple(
  x264_proto::session_params_t value)
{
  return tuple_t(
    value.common_,
    value.profile_idc_,
    value.level_idc_,
    value.vui_overscan_appropriate_flag_,
    value.vui_video_format_,
    value.vui_video_full_range_flag_,
    value.vui_colour_primaries_,
    value.vui_transfer_characteristics_,
    value.vui_matrix_coefficients_,
    value.vui_chroma_sample_loc_type_top_field_,
    value.vui_chroma_sample_loc_type_bottom_field_,
    value.vui_num_units_in_tick_,
    value.vui_time_scale_,
    value.vui_fixed_frame_rate_flag_);
}

x264_proto::session_params_t
cuti::tuple_mapping_t<x264_proto::session_params_t>::from_tuple(tuple_t tuple)
{
  x264_proto::session_params_t value;
  value.common_ = std::get<0>(tuple);
  value.profile_idc_ = std::get<1>(tuple);
  value.level_idc_ = std::get<2>(tuple);
  value.vui_overscan_appropriate_flag_ = std::get<3>(tuple);
  value.vui_video_format_ = std::get<4>(tuple);
  value.vui_video_full_range_flag_ = std::get<5>(tuple);
  value.vui_colour_primaries_ = std::get<6>(tuple);
  value.vui_transfer_characteristics_ = std::get<7>(tuple);
  value.vui_matrix_coefficients_ = std::get<8>(tuple);
  value.vui_chroma_sample_loc_type_top_field_ = std::get<9>(tuple);
  value.vui_chroma_sample_loc_type_bottom_field_ = std::get<10>(tuple);
  value.vui_num_units_in_tick_ = std::get<11>(tuple);
  value.vui_time_scale_ = std::get<12>(tuple);
  value.vui_fixed_frame_rate_flag_ = std::get<13>(tuple);
  return value;
}

cuti::tuple_mapping_t<x264_proto::sample_headers_t>::tuple_t
cuti::tuple_mapping_t<x264_proto::sample_headers_t>::to_tuple(
  x264_proto::sample_headers_t value)
{
  return tuple_t(std::move(value.sps_), std::move(value.pps_));
}

x264_proto::sample_headers_t
cuti::tuple_mapping_t<x264_proto::sample_headers_t>::from_tuple(tuple_t tuple)
{
  x264_proto::sample_headers_t value;
  value.sps_ = std::move(std::get<0>(tuple));
  value.pps_ = std::move(std::get<1>(tuple));
  return value;
}
