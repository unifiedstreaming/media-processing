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

#include <tuple>
#include <utility>

namespace x264_proto
{

struct X264_PROTO_ABI session_params_t
{
  // TBD
};

struct X264_PROTO_ABI frame_t
{
  // TBD
};

struct X264_PROTO_ABI sample_headers_t
{
  // TBD
};

struct X264_PROTO_ABI sample_t
{
  // TBD
};

} // x264_proto

// cuti serialization

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
