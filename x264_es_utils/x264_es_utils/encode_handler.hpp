/*
 * Copyright (C) 2024-2025 CodeShop B.V.
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

#ifndef X264_ES_UTILS_ENCODE_HANDLER_HPP_
#define X264_ES_UTILS_ENCODE_HANDLER_HPP_

#include "encoder_settings.hpp"
#include "encoding_session.hpp"

#include <cuti/async_readers.hpp>
#include <cuti/async_writers.hpp>
#include <cuti/bound_inbuf.hpp>
#include <cuti/bound_outbuf.hpp>
#include <cuti/logging_context.hpp>
#include <cuti/result.hpp>
#include <cuti/stack_marker.hpp>
#include <cuti/subroutine.hpp>

#include <x264_proto/types.hpp>

#include <optional>

namespace x264_es_utils
{

struct encode_handler_t
{
  using result_value_t = void;

  encode_handler_t(cuti::result_t<void>& result,
                   cuti::logging_context_t const& context,
		   cuti::bound_inbuf_t& inbuf,
		   cuti::bound_outbuf_t& outbuf,
		   encoder_settings_t encoder_settings);

  encode_handler_t(encode_handler_t const&) = delete;
  encode_handler_t& operator=(encode_handler_t const&) = delete;
  
  void start(cuti::stack_marker_t& marker);

private :
  void create_session(
    cuti::stack_marker_t& marker,
    x264_proto::session_params_t session_params);
  void read_begin_sequence(cuti::stack_marker_t& marker);
  void write_begin_sequence(cuti::stack_marker_t& marker);

  void check_eos(cuti::stack_marker_t& marker);
  void handle_eos_check(cuti::stack_marker_t& marker, bool at_end);
  void encode_frame(cuti::stack_marker_t& marker, x264_proto::frame_t frame);
  void flush_samples(cuti::stack_marker_t& marker);
  void report_success(cuti::stack_marker_t& marker);
  
private :
  cuti::result_t<void>& result_;
  cuti::logging_context_t const& context_;
  encoder_settings_t encoder_settings_;
  std::optional<encoding_session_t> encoding_session_;

  cuti::subroutine_t<encode_handler_t,
    cuti::reader_t<x264_proto::session_params_t>> session_params_reader_;
  cuti::subroutine_t<encode_handler_t,
    cuti::writer_t<x264_proto::sample_headers_t>> sample_headers_writer_;

  cuti::subroutine_t<encode_handler_t,
    cuti::begin_sequence_reader_t> begin_sequence_reader_;
  cuti::subroutine_t<encode_handler_t,
    cuti::begin_sequence_writer_t> begin_sequence_writer_;

  cuti::subroutine_t<encode_handler_t,
    cuti::end_sequence_checker_t> end_sequence_checker_;
  cuti::subroutine_t<encode_handler_t,
    cuti::reader_t<x264_proto::frame_t>> frame_reader_;
  cuti::subroutine_t<encode_handler_t,
    cuti::writer_t<x264_proto::sample_t>> sample_writer_;
  cuti::subroutine_t<encode_handler_t,
    cuti::end_sequence_writer_t> end_sequence_writer_;
};

} // x264_es_utils

#endif
