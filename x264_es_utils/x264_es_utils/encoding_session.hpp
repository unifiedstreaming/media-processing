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

#ifndef X264_ES_UTILS_ENCODING_SESSION_HPP_
#define X264_ES_UTILS_ENCODING_SESSION_HPP_

#include "encoder_settings.hpp"

#include <cuti/exception_builder.hpp>
#include <cuti/logging_context.hpp>

#include <x264_proto/types.hpp>

#include <optional>

namespace x264_es_utils
{

struct x264_exception_t : std::runtime_error
{
  explicit x264_exception_t(std::string complaint);
  ~x264_exception_t() override;
};

using x264_exception_builder_t = cuti::exception_builder_t<x264_exception_t>;

struct encoding_session_t
{
  encoding_session_t(cuti::logging_context_t const& logging_context,
                     encoder_settings_t const& encoder_settings,
                     x264_proto::session_params_t const& session_params);

  encoding_session_t(encoding_session_t const&) = delete;
  encoding_session_t& operator=(encoding_session_t const&) = delete;

  x264_proto::sample_headers_t sample_headers() const;

  std::optional<x264_proto::sample_t> encode(x264_proto::frame_t frame);
  std::optional<x264_proto::sample_t> flush();

  ~encoding_session_t();

private :
  struct impl_t;
  std::unique_ptr<impl_t> impl_;
};

} // x264_es_utils

#endif
