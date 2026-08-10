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

#include "types.hpp"

#include <cuti/exception_builder.hpp>
#include <cuti/parse_error.hpp>

namespace x26x_proto
{

std::string to_string(format_t format)
{
  switch(format)
  {
  case format_t::NV12:
    return "NV12";
  case format_t::YUV420P:
    return "YUV420P";
  case format_t::YUV420P10LE:
    return "YUV420P10LE";
  default:
    return "bad x26x_proto::format_t value " +
      std::to_string(cuti::to_underlying(format));
  }
}

common_session_params_t::common_session_params_t()
: timescale_(0)
, bitrate_(0)
, width_(0)
, height_(0)
, sar_width_(1)
, sar_height_(1)
, format_(format_t::NV12)
, framerate_(std::nullopt)
{
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

std::size_t frame_size(uint32_t width, uint32_t height, format_t format)
{
  return static_cast<std::size_t>(width) * height * 3 /
    (format == format_t::YUV420P10LE ? 1 : 2);
}

sample_t::sample_t()
: dts_(0)
, pts_(0)
, type_(type_t::i)
, data_()
{
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
    return "unknown x26x_proto::sample_t::type_t value " +
      std::to_string(cuti::to_underlying(type));
  }
}
} // x26x_proto

x26x_proto::format_t
cuti::enum_mapping_t<x26x_proto::format_t>::from_underlying(underlying_t value)
{
  switch(value)
  {
  case to_underlying(x26x_proto::format_t::NV12):
  case to_underlying(x26x_proto::format_t::YUV420P):
  case to_underlying(x26x_proto::format_t::YUV420P10LE):
    return x26x_proto::format_t{value};
  default:
    exception_builder_t<parse_error_t> builder;
    builder << "bad x26x_proto::format_t value " << to_serialized(value);
    builder.explode();
  }
}

cuti::tuple_mapping_t<x26x_proto::common_session_params_t>::tuple_t
cuti::tuple_mapping_t<x26x_proto::common_session_params_t>::to_tuple(
  x26x_proto::common_session_params_t value)
{
  return tuple_t(
    value.timescale_,
    value.bitrate_,
    value.width_,
    value.height_,
    value.sar_width_,
    value.sar_height_,
    value.format_,
    value.framerate_);
}

x26x_proto::common_session_params_t
cuti::tuple_mapping_t<x26x_proto::common_session_params_t>::from_tuple(tuple_t tuple)
{
  x26x_proto::common_session_params_t value;
  value.timescale_ = std::get<0>(tuple);
  value.bitrate_ = std::get<1>(tuple);
  value.width_ = std::get<2>(tuple);
  value.height_ = std::get<3>(tuple);
  value.sar_width_ = std::get<4>(tuple);
  value.sar_height_ = std::get<5>(tuple);
  value.format_ = std::get<6>(tuple);
  value.framerate_ = std::get<7>(tuple);
  return value;
}

cuti::tuple_mapping_t<x26x_proto::frame_t>::tuple_t
cuti::tuple_mapping_t<x26x_proto::frame_t>::to_tuple(x26x_proto::frame_t value)
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

x26x_proto::frame_t
cuti::tuple_mapping_t<x26x_proto::frame_t>::from_tuple(tuple_t tuple)
{
  x26x_proto::frame_t value;
  value.width_ = std::get<0>(tuple);
  value.height_ = std::get<1>(tuple);
  value.format_ = std::get<2>(tuple);
  value.pts_ = std::get<3>(tuple);
  value.timescale_ = std::get<4>(tuple);
  value.keyframe_ = std::get<5>(tuple);
  value.data_ = std::move(std::get<6>(tuple));
  return value;
}

x26x_proto::sample_t::type_t
cuti::enum_mapping_t<x26x_proto::sample_t::type_t>::from_underlying(
  underlying_t value)
{
  switch(value)
  {
  case to_underlying(x26x_proto::sample_t::type_t::i):
  case to_underlying(x26x_proto::sample_t::type_t::p):
  case to_underlying(x26x_proto::sample_t::type_t::b):
  case to_underlying(x26x_proto::sample_t::type_t::b_ref):
    return x26x_proto::sample_t::type_t{value};
  default:
    exception_builder_t<parse_error_t> builder;
    builder << "bad x26x_proto::sample_t::type_t value " <<
      to_serialized(value);
    builder.explode();
  }
}

cuti::tuple_mapping_t<x26x_proto::sample_t>::tuple_t
cuti::tuple_mapping_t<x26x_proto::sample_t>::to_tuple(
  x26x_proto::sample_t value)
{
  return tuple_t(
    value.dts_,
    value.pts_,
    value.type_,
    std::move(value.data_));
}

x26x_proto::sample_t
cuti::tuple_mapping_t<x26x_proto::sample_t>::from_tuple(tuple_t tuple)
{
  x26x_proto::sample_t value;
  value.dts_ = std::get<0>(tuple);
  value.pts_ = std::get<1>(tuple);
  value.type_ = std::get<2>(tuple);
  value.data_ = std::move(std::get<3>(tuple));
  return value;
}
