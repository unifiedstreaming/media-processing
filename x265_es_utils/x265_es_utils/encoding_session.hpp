/*
 * Copyright (C) 2026 CodeShop B.V.
 *
 * This file is part of the x265_es_utils library.
 *
 * The x265_es_utils library is free software: you can redistribute it
 * and/or modify it under the terms of version 2 of the GNU General
 * Public License as published by the Free Software Foundation.
 *
 * The x265_es_utils library is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See version 2 of the GNU General Public License for more details.
 *
 * You should have received a copy of version 2 of the GNU General
 * Public License along with the x265_es_utils library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef X265_ES_UTILS_ENCODING_SESSION_HPP_
#define X265_ES_UTILS_ENCODING_SESSION_HPP_

#include "encoder_settings.hpp"

#include <cuti/exception_builder.hpp>
#include <cuti/logging_context.hpp>

#include <x265_proto/types.hpp>

#include <optional>

namespace x265_es_utils
{

struct x265_exception_t : std::runtime_error
{
  explicit x265_exception_t(std::string complaint);
  ~x265_exception_t() override;
};

using x265_exception_builder_t = cuti::exception_builder_t<x265_exception_t>;

struct encoding_session_t
{
  encoding_session_t(cuti::logging_context_t const& logging_context,
                     encoder_settings_t const& encoder_settings,
                     x265_proto::session_params_t const& session_params);

  encoding_session_t(encoding_session_t const&) = delete;
  encoding_session_t& operator=(encoding_session_t const&) = delete;

  x265_proto::sample_headers_t sample_headers() const;

  std::optional<x26x_proto::sample_t> encode(x26x_proto::frame_t frame);
  std::optional<x26x_proto::sample_t> flush();

  ~encoding_session_t();

private :
  struct impl_t;
  std::unique_ptr<impl_t> impl_;
};

} // x265_es_utils

#endif
