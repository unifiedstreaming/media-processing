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

#include <string>
#include <tuple>
#include <utility>

namespace x264_proto
{

enum class profile_t : unsigned
{
  BASELINE = 66,
  MAIN = 77,
  HIGH = 100,
  HIGH10 = 110,
  HIGH422 = 122,
  HIGH444_PREDICTIVE = 244,
};

enum class format_t : unsigned
{
  NV12,
};

struct X264_PROTO_ABI session_params_t
{
  // TBD

  bool operator==(session_params_t const& rhs) const
  { return true; }
  
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
struct X264_PROTO_ABI cuti::tuple_mapping_t<x264_proto::profile_t>
{
  using underlying_t = std::underlying_type_t<x264_proto::profile_t>;
  using tuple_t = std::tuple<underlying_t>;

  static tuple_t to_tuple(x264_proto::profile_t value)
  {
    return {static_cast<underlying_t>(value)};
  }

  static x264_proto::profile_t from_tuple(tuple_t tuple)
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
};

template<>
struct X264_PROTO_ABI cuti::tuple_mapping_t<x264_proto::format_t>
{
  using underlying_t = std::underlying_type_t<x264_proto::format_t>;
  using tuple_t = std::tuple<underlying_t>;

  static tuple_t to_tuple(x264_proto::format_t value)
  {
    return {static_cast<underlying_t>(value)};
  }

  static x264_proto::format_t from_tuple(tuple_t tuple)
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
};

template<>
struct X264_PROTO_ABI cuti::tuple_mapping_t<x264_proto::session_params_t>
{
  using tuple_t = std::tuple<>; // TBD

  static tuple_t to_tuple(x264_proto::session_params_t value)
  {
    return tuple_t();
  }

  static x264_proto::session_params_t from_tuple(tuple_t tuple)
  {
    return
      std::make_from_tuple<x264_proto::session_params_t>(std::move(tuple));
  }
};

template<>
struct X264_PROTO_ABI cuti::tuple_mapping_t<x264_proto::frame_t>
{
  using tuple_t = std::tuple<>; // TBD

  static tuple_t to_tuple(x264_proto::frame_t value)
  {
    return tuple_t();
  }

  static x264_proto::frame_t from_tuple(tuple_t tuple)
  {
    return
      std::make_from_tuple<x264_proto::frame_t>(std::move(tuple));
  }
};

template<>
struct X264_PROTO_ABI cuti::tuple_mapping_t<x264_proto::sample_headers_t>
{
  using tuple_t = std::tuple<>; // TBD

  static tuple_t to_tuple(x264_proto::sample_headers_t value)
  {
    return tuple_t();
  }

  static x264_proto::sample_headers_t from_tuple(tuple_t tuple)
  {
    return
      std::make_from_tuple<x264_proto::sample_headers_t>(std::move(tuple));
  }
};

template<>
struct X264_PROTO_ABI cuti::tuple_mapping_t<x264_proto::sample_t>
{
  using tuple_t = std::tuple<>; // TBD

  static tuple_t to_tuple(x264_proto::sample_t value)
  {
    return tuple_t();
  }

  static x264_proto::sample_t from_tuple(tuple_t tuple)
  {
    return
      std::make_from_tuple<x264_proto::sample_t>(std::move(tuple));
  }
};

#endif
