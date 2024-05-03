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

#include "types.hpp"

namespace x264_proto
{

session_params_t::session_params_t(
  unsigned timescale,
  unsigned bitrate,
  unsigned width,
  unsigned height,
  unsigned sar_width,
  unsigned sar_height,
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
  unsigned framerate_den)
: timescale_(timescale)
, bitrate_(bitrate)
, width_(width)
, height_(height)
, sar_width_(sar_width)
, sar_height_(sar_height)
, profile_idc_(profile_idc)
, level_idc_(level_idc)
, overscan_appropriate_flag_(overscan_appropriate_flag)
, video_format_(video_format)
, video_full_range_flag_(video_full_range_flag)
, colour_primaries_(colour_primaries)
, transfer_characteristics_(transfer_characteristics)
, matrix_coefficients_(matrix_coefficients)
, chroma_sample_loc_type_top_field_(chroma_sample_loc_type_top_field)
, chroma_sample_loc_type_bottom_field_(chroma_sample_loc_type_bottom_field)
, framerate_num_(framerate_num)
, framerate_den_(framerate_den)
{
}

bool session_params_t::operator==(session_params_t const& rhs) const
{
  return timescale_ == rhs.timescale_
    && bitrate_ == rhs.bitrate_
    && width_ == rhs.width_
    && height_ == rhs.height_
    && sar_width_ == rhs.sar_width_
    && sar_height_ == rhs.sar_height_
    && profile_idc_ == rhs.profile_idc_
    && level_idc_ == rhs.level_idc_
    && overscan_appropriate_flag_ == rhs.overscan_appropriate_flag_
    && video_format_ == rhs.video_format_
    && video_full_range_flag_ == rhs.video_full_range_flag_
    && colour_primaries_ == rhs.colour_primaries_
    && transfer_characteristics_ == rhs.transfer_characteristics_
    && matrix_coefficients_ == rhs.matrix_coefficients_
    && chroma_sample_loc_type_top_field_ == rhs.chroma_sample_loc_type_top_field_
    && chroma_sample_loc_type_bottom_field_ == rhs.chroma_sample_loc_type_bottom_field_
    && framerate_num_ == rhs.framerate_num_
    && framerate_den_ == rhs.framerate_den_;
}

} // x264_proto

x264_proto::profile_t
cuti::tuple_mapping_t<x264_proto::profile_t>::from_tuple(tuple_t tuple)
{
  auto const& value = std::get<0>(tuple);
  switch(value)
  {
  case static_cast<underlying_t>(x264_proto::profile_t::BASELINE):
  case static_cast<underlying_t>(x264_proto::profile_t::MAIN):
  case static_cast<underlying_t>(x264_proto::profile_t::HIGH):
  case static_cast<underlying_t>(x264_proto::profile_t::HIGH10):
  case static_cast<underlying_t>(x264_proto::profile_t::HIGH422):
  case static_cast<underlying_t>(x264_proto::profile_t::HIGH444_PREDICTIVE):
    return static_cast<x264_proto::profile_t>(value);
  default:
    throw parse_error_t("bad x264_proto::profile_t value " + std::to_string(value));
  }
}

x264_proto::format_t
cuti::tuple_mapping_t<x264_proto::format_t>::from_tuple(tuple_t tuple)
{
  auto const& value = std::get<0>(tuple);
  switch(value)
  {
  case static_cast<underlying_t>(x264_proto::format_t::NV12):
    return static_cast<x264_proto::format_t>(value);
  default:
    throw parse_error_t("bad x264_proto::format_t value " + std::to_string(value));
  }
}

cuti::tuple_mapping_t<x264_proto::session_params_t>::tuple_t
cuti::tuple_mapping_t<x264_proto::session_params_t>::to_tuple(x264_proto::session_params_t value)
{
  return tuple_t(
   value.timescale_,
   value.bitrate_,
   value.width_,
   value.height_,
   value.sar_width_,
   value.sar_height_,
   value.profile_idc_,
   value.level_idc_,
   value.overscan_appropriate_flag_,
   value.video_format_,
   value.video_full_range_flag_,
   value.colour_primaries_,
   value.transfer_characteristics_,
   value.matrix_coefficients_,
   value.chroma_sample_loc_type_top_field_,
   value.chroma_sample_loc_type_bottom_field_,
   value.framerate_num_,
   value.framerate_den_);
}
