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

#include <cuti/exception_builder.hpp>
#include <cuti/parse_error.hpp>

namespace x264_proto
{

std::string to_string(format_t format)
{
  switch(format)
  {
  case format_t::NV12:
    return "NV12";
  default:
    return "bad x264_proto::format_t value " +
      std::to_string(cuti::to_underlying(format));
  }
}

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
: timescale_(0)
, bitrate_(0)
, width_(0)
, height_(0)
, sar_width_(1)
, sar_height_(1)
, format_(format_t::NV12)
, profile_idc_(profile_t::BASELINE)
, level_idc_(0)
, overscan_appropriate_flag_(std::nullopt)
, video_format_(std::nullopt)
, video_full_range_flag_(std::nullopt)
, colour_primaries_(std::nullopt)
, transfer_characteristics_(std::nullopt)
, matrix_coefficients_(std::nullopt)
, chroma_sample_loc_type_top_field_(std::nullopt)
, chroma_sample_loc_type_bottom_field_(std::nullopt)
, framerate_num_(25)
, framerate_den_(1)
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
    && format_ == rhs.format_
    && profile_idc_ == rhs.profile_idc_
    && level_idc_ == rhs.level_idc_
    && overscan_appropriate_flag_ == rhs.overscan_appropriate_flag_
    && video_format_ == rhs.video_format_
    && video_full_range_flag_ == rhs.video_full_range_flag_
    && colour_primaries_ == rhs.colour_primaries_
    && transfer_characteristics_ == rhs.transfer_characteristics_
    && matrix_coefficients_ == rhs.matrix_coefficients_
    && chroma_sample_loc_type_top_field_ ==
         rhs.chroma_sample_loc_type_top_field_
    && chroma_sample_loc_type_bottom_field_ ==
         rhs.chroma_sample_loc_type_bottom_field_
    && framerate_num_ == rhs.framerate_num_
    && framerate_den_ == rhs.framerate_den_;
}

frame_t::frame_t()
: width_(0)
, height_(0)
, format_(format_t::NV12)
, pts_(0)
, timescale_(0)
, keyframe_(false)
, data_()
{
}

bool frame_t::operator==(frame_t const& rhs) const
{
  return width_ == rhs.width_
    && height_ == rhs.height_
    && format_ == rhs.format_
    && pts_ == rhs.pts_
    && timescale_ == rhs.timescale_
    && keyframe_ == rhs.keyframe_
    && data_ == rhs.data_;
}

sample_headers_t::sample_headers_t()
: sps_()
, pps_()
{
}

bool sample_headers_t::operator==(sample_headers_t const& rhs) const
{
  return sps_ == rhs.sps_
    && pps_ == rhs.pps_;
}

sample_t::sample_t()
: dts_(0)
, pts_(0)
, type_(type_t::i)
, data_()
{
}

bool sample_t::operator==(sample_t const& rhs) const
{
  return dts_ == rhs.dts_
    && pts_ == rhs.pts_
    && type_ == rhs.type_
    && data_ == rhs.data_;
}

std::string to_string(sample_t::type_t type)
{
  switch(type)
  {
  case sample_t::type_t::i:
    return "I";
  case sample_t::type_t::p:
    return "P";
  case sample_t::type_t::b:
    return "B";
  case sample_t::type_t::b_ref:
    return "B_ref";
  default:
    return "unknown x264_proto::sample_t::type_t value " +
      std::to_string(cuti::to_underlying(type));
  }
}
} // x264_proto

x264_proto::format_t
cuti::enum_mapping_t<x264_proto::format_t>::from_underlying(underlying_t value)
{
  switch(value)
  {
  case to_underlying(x264_proto::format_t::NV12):
    return x264_proto::format_t{value};
  default:
    exception_builder_t<parse_error_t> builder;
    builder << "bad x264_proto::format_t value " << to_serialized(value);
    builder.explode();
  }
}

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
   value.timescale_,
   value.bitrate_,
   value.width_,
   value.height_,
   value.sar_width_,
   value.sar_height_,
   value.format_,
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

x264_proto::session_params_t
cuti::tuple_mapping_t<x264_proto::session_params_t>::from_tuple(tuple_t tuple)
{
  x264_proto::session_params_t value;
  value.timescale_ = std::get<0>(tuple);
  value.bitrate_ = std::get<1>(tuple);
  value.width_ = std::get<2>(tuple);
  value.height_ = std::get<3>(tuple);
  value.sar_width_ = std::get<4>(tuple);
  value.sar_height_ = std::get<5>(tuple);
  value.format_ = std::get<6>(tuple);
  value.profile_idc_ = std::get<7>(tuple);
  value.level_idc_ = std::get<8>(tuple);
  value.overscan_appropriate_flag_ = std::get<9>(tuple);
  value.video_format_ = std::get<10>(tuple);
  value.video_full_range_flag_ = std::get<11>(tuple);
  value.colour_primaries_ = std::get<12>(tuple);
  value.transfer_characteristics_ = std::get<13>(tuple);
  value.matrix_coefficients_ = std::get<14>(tuple);
  value.chroma_sample_loc_type_top_field_ = std::get<15>(tuple);
  value.chroma_sample_loc_type_bottom_field_ = std::get<16>(tuple);
  value.framerate_num_ = std::get<17>(tuple);
  value.framerate_den_ = std::get<18>(tuple);
  return value;
}

cuti::tuple_mapping_t<x264_proto::frame_t>::tuple_t
cuti::tuple_mapping_t<x264_proto::frame_t>::to_tuple(x264_proto::frame_t value)
{
  return tuple_t(
    value.width_,
    value.height_,
    value.format_,
    value.pts_,
    value.timescale_,
    value.keyframe_,
    std::move(value.data_));
}

x264_proto::frame_t
cuti::tuple_mapping_t<x264_proto::frame_t>::from_tuple(tuple_t tuple)
{
  x264_proto::frame_t value;
  value.width_ = std::get<0>(tuple);
  value.height_ = std::get<1>(tuple);
  value.format_ = std::get<2>(tuple);
  value.pts_ = std::get<3>(tuple);
  value.timescale_ = std::get<4>(tuple);
  value.keyframe_ = std::get<5>(tuple);
  value.data_ = std::move(std::get<6>(tuple));
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

x264_proto::sample_t::type_t
cuti::enum_mapping_t<x264_proto::sample_t::type_t>::from_underlying(
  underlying_t value)
{
  switch(value)
  {
  case to_underlying(x264_proto::sample_t::type_t::i):
  case to_underlying(x264_proto::sample_t::type_t::p):
  case to_underlying(x264_proto::sample_t::type_t::b):
  case to_underlying(x264_proto::sample_t::type_t::b_ref):
    return x264_proto::sample_t::type_t{value};
  default:
    exception_builder_t<parse_error_t> builder;
    builder << "bad x264_proto::sample_t::type_t value " <<
      to_serialized(value);
    builder.explode();
  }
}

cuti::tuple_mapping_t<x264_proto::sample_t>::tuple_t
cuti::tuple_mapping_t<x264_proto::sample_t>::to_tuple(
  x264_proto::sample_t value)
{
  return tuple_t(
    value.dts_,
    value.pts_,
    value.type_,
    std::move(value.data_));
}

x264_proto::sample_t
cuti::tuple_mapping_t<x264_proto::sample_t>::from_tuple(tuple_t tuple)
{
  x264_proto::sample_t value;
  value.dts_ = std::get<0>(tuple);
  value.pts_ = std::get<1>(tuple);
  value.type_ = std::get<2>(tuple);
  value.data_ = std::move(std::get<3>(tuple));
  return value;
}
