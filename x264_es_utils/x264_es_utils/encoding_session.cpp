/*
 * Copyright (C) 2024 CodeShop B.V.
 *
 * This file is part of the x264_es_utils library.
 *
 * The x264_es_utils library is free software: you can redistribute it
 * and/or modify it under the terms of version 2 of the GNU General
 * Public License as published by the Free Software Foundation.
 *
 * The x264_es_utils library is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See version 2 of the GNU General Public License for more details.
 *
 * You should have received a copy of version 2 of the GNU General
 * Public License along with the x264_es_utils library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "encoding_session.hpp"

#include <cassert>

namespace x264_es_utils
{

encoding_session_t::encoding_session_t(encoder_settings_t const&,
                                       x264_proto::session_params_t const&)
: backlog_(0)
, flush_called_(false)
{ }

x264_proto::samples_header_t encoding_session_t::samples_header() const
{
  return x264_proto::samples_header_t();
}

std::optional<x264_proto::sample_t>
encoding_session_t::encode(x264_proto::frame_t)
{
  assert(! flush_called_);
  
  std::optional<x264_proto::sample_t> result = std::nullopt;

  if(backlog_ == 3)
  {
    result.emplace();
  }
  else
  {
    ++backlog_;
  }
  
  return result;
}

std::optional<x264_proto::sample_t>
encoding_session_t::flush()
{
  flush_called_ = true;
  
  std::optional<x264_proto::sample_t> result = std::nullopt;

  if(backlog_ > 0)
  {
    result.emplace();
    --backlog_;
  }
  
  return result;
}

} // x264_es_utils
