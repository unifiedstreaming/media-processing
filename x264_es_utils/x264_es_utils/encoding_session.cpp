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

#include <cuti/exception_builder.hpp>
#include <x264_proto/types.hpp>

#include <cassert>
#include <x264.h>

namespace x264_es_utils
{

namespace // anonymous
{

struct x264_deleter_t
{
  void operator()(x264_t* encoder) const
  {
    x264_encoder_close(encoder);
  }
};

using x264_handle_t = std::unique_ptr<x264_t, x264_deleter_t>;

struct wrap_x264_param_t
{
  wrap_x264_param_t(cuti::logging_context_t const& logging_context,
                    x264_proto::session_params_t const& session_params);

  wrap_x264_param_t(wrap_x264_param_t const&) = delete;
  wrap_x264_param_t operator=(wrap_x264_param_t const&) = delete;

  void print(std::ostream& os) const;

  x264_handle_t create_x264_handle();

  ~wrap_x264_param_t();

private :
  explicit wrap_x264_param_t(cuti::logging_context_t const& logging_context);

private :
  cuti::logging_context_t const& logging_context_;
  x264_param_t param_;
};

inline
std::ostream& operator<<(std::ostream& os, wrap_x264_param_t const& rhs)
{
  rhs.print(os);
  return os;
}

struct x264_output_t
{
  x264_output_t()
  : nals_(nullptr)
  , n_nals_(0)
  {
    std::memset(&pic_, 0, sizeof pic_);
  }

  x264_output_t(x264_output_t const&) = delete;
  x264_output_t& operator=(x264_output_t const&) = delete;

  x264_nal_t* nals_;
  int n_nals_;
  x264_picture_t pic_;
};

struct input_picture_t
{
public:
  input_picture_t(cuti::logging_context_t const& context,
                  x264_proto::frame_t const& frame);

  input_picture_t(input_picture_t const&) = delete;
  input_picture_t& operator=(input_picture_t const&) = delete;

  int encode(x264_t& encoder, x264_output_t& output);

  void print(std::ostream& os) const;

  ~input_picture_t();

private :
  cuti::logging_context_t const& logging_context_;
  x264_picture_t picture_;
};

inline
std::ostream& operator<<(std::ostream& os, input_picture_t const& rhs)
{
  rhs.print(os);
  return os;
}

struct wrap_x264_encoder_t
{
  wrap_x264_encoder_t(cuti::logging_context_t const& logging_context,
                      x264_proto::session_params_t const& session_params);

  wrap_x264_encoder_t(const wrap_x264_encoder_t&) = delete;
  wrap_x264_encoder_t& operator=(const wrap_x264_encoder_t&) = delete;

  int delayed_frames() const;

  int encode(x264_output_t& output, input_picture_t& pic_in);

  int flush(x264_output_t& output);

  ~wrap_x264_encoder_t();

private :
  static x264_handle_t
  create_x264_handle(cuti::logging_context_t const& synchronizer,
                     x264_proto::session_params_t const& session_params);

private :
  cuti::logging_context_t const& logging_context_;
  x264_handle_t handle_;
};

} // anonymous namespace

x264_exception_t::x264_exception_t(std::string complaint)
: std::runtime_error(std::move(complaint))
{ }

x264_exception_t::~x264_exception_t()
{ }

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

x264_proto::sample_headers_t encoding_session_t::sample_headers() const
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
        "returning encoded sample";
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
