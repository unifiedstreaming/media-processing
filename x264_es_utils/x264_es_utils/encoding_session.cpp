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

encoding_session_t::encoding_session_t(
  cuti::logging_context_t const& context,
  encoder_settings_t const&,
  x264_proto::session_params_t const&)
: context_(context)
, backlog_(0)
, flush_called_(false)
{
  if(auto msg = context_.message_at(cuti::loglevel_t::info))
  {
    *msg << "encoding_session[" << this << "]: created";
  }
}

x264_proto::sample_headers_t encoding_session_t::samples_header() const
{
  if(auto msg = context_.message_at(cuti::loglevel_t::info))
  {
    *msg << "encoding_session[" << this << "]: returning samples header";
  }

  return x264_proto::sample_headers_t();
}

std::optional<x264_proto::sample_t>
encoding_session_t::encode(x264_proto::frame_t)
{
  assert(! flush_called_);
  
  std::optional<x264_proto::sample_t> result = std::nullopt;

  if(backlog_ == 3)
  {
    result.emplace();
    if(auto msg = context_.message_at(cuti::loglevel_t::info))
    {
      *msg << "encoding_session[" << this << "]: encode: " <<
        "returning encoded sample";
    }
  }
  else
  {
    ++backlog_;
    if(auto msg = context_.message_at(cuti::loglevel_t::info))
    {
      *msg << "encoding_session[" << this << "]: encode: " <<
        "no sample available";
    }
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
    if(auto msg = context_.message_at(cuti::loglevel_t::info))
    {
      *msg << "encoding_session[" << this << "]: flush: " <<
        "returning encode sample";
    }
  }
  else
  {
    if(auto msg = context_.message_at(cuti::loglevel_t::info))
    {
      *msg << "encoding_session[" << this << "]: flush: " <<
        "all samples flushed";
    }
  }
  
  return result;
}

encoding_session_t::~encoding_session_t()
{
  if(auto msg = context_.message_at(cuti::loglevel_t::info))
  {
    *msg << "encoding_session[" << this << "]: destroying";
  }
}

} // x264_es_utils
