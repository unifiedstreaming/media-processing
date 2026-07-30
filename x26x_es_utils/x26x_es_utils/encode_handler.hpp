/*
 * Copyright (C) 2026 CodeShop B.V.
 *
 * This file is part of the x26x_es_utils library.
 *
 * The x26x_es_utils library is free software: you can redistribute it
 * and/or modify it under the terms of version 2 of the GNU General
 * Public License as published by the Free Software Foundation.
 *
 * The x26x_es_utils library is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See version 2 of the GNU General Public License for more details.
 *
 * You should have received a copy of version 2 of the GNU General
 * Public License along with the x26x_es_utils library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef X26X_ES_UTILS_ENCODE_HANDLER_HPP_
#define X26X_ES_UTILS_ENCODE_HANDLER_HPP_

#include <cuti/async_readers.hpp>
#include <cuti/async_writers.hpp>
#include <cuti/bound_inbuf.hpp>
#include <cuti/bound_outbuf.hpp>
#include <cuti/logging_context.hpp>
#include <cuti/result.hpp>
#include <cuti/stack_marker.hpp>
#include <cuti/subroutine.hpp>

#include <x26x_proto/types.hpp>

#include <cassert>
#include <exception>
#include <optional>
#include <utility>

namespace x26x_es_utils
{

template<typename EncoderSettings, typename EncodingSession,
         typename SessionParams, typename SampleHeaders>
struct encode_handler_t
{
  using result_value_t = void;

  encode_handler_t(cuti::result_t<void>& result,
                   cuti::logging_context_t const& context,
		   cuti::bound_inbuf_t& inbuf,
		   cuti::bound_outbuf_t& outbuf,
		   EncoderSettings encoder_settings)
  : result_(result)
  , context_(context)
  , encoder_settings_(std::move(encoder_settings))
  , encoding_session_(std::nullopt)
  , session_params_reader_(*this, result_, inbuf)
  , sample_headers_writer_(*this, result_, outbuf)
  , begin_sequence_reader_(*this, result_, inbuf)
  , begin_sequence_writer_(*this, result_, outbuf)
  , end_sequence_checker_(*this, result_, inbuf)
  , frame_reader_(*this, result_, inbuf)
  , sample_writer_(*this, result_, outbuf)
  , end_sequence_writer_(*this, result_, outbuf)
  { }

  encode_handler_t(encode_handler_t const&) = delete;
  encode_handler_t& operator=(encode_handler_t const&) = delete;
  
  void start(cuti::stack_marker_t& marker)
  {
    session_params_reader_.start(marker, &encode_handler_t::create_session);
  }

private :
  void create_session(
    cuti::stack_marker_t& marker,
    SessionParams session_params)
  {
    try
    {
      encoding_session_.emplace(context_, encoder_settings_, session_params);
    }
    catch(std::exception const&)
    {
      result_.fail(marker, std::current_exception());
      return;
    }

    sample_headers_writer_.start(
      marker,
      &encode_handler_t::read_begin_sequence,
      encoding_session_->sample_headers());
  }

  void read_begin_sequence(cuti::stack_marker_t& marker)
  {
    begin_sequence_reader_.start(
      marker,
      &encode_handler_t::write_begin_sequence);
  }

  void write_begin_sequence(cuti::stack_marker_t& marker)
  {
    begin_sequence_writer_.start(marker, &encode_handler_t::check_eos);
  }

  void check_eos(cuti::stack_marker_t& marker)
  {
    end_sequence_checker_.start(marker, &encode_handler_t::handle_eos_check);
  }

  void handle_eos_check(cuti::stack_marker_t& marker, bool at_end)
  {
    if(! at_end)
    {
      frame_reader_.start(marker, &encode_handler_t::encode_frame);
    }
    else
    {
      this->flush_samples(marker);
    }
  }

  void encode_frame(cuti::stack_marker_t& marker, x26x_proto::frame_t frame)
  {
    assert(encoding_session_ != std::nullopt);

    std::optional<x26x_proto::sample_t> opt_sample;
    try
    {
      opt_sample = encoding_session_->encode(std::move(frame));
    }
    catch(std::exception const&)
    {
      result_.fail(marker, std::current_exception());
      return;
    }

    if(opt_sample)
    {
      sample_writer_.start(
        marker,
        &encode_handler_t::check_eos,
        std::move(*opt_sample));
    }
    else
    {
      this->check_eos(marker);
    }
  }

  void flush_samples(cuti::stack_marker_t& marker)
  {
    assert(encoding_session_ != std::nullopt);

    std::optional<x26x_proto::sample_t> opt_sample;
    try
    {
      opt_sample = encoding_session_->flush();
    }
    catch(std::exception const&)
    {
      result_.fail(marker, std::current_exception());
      return;
    }

    if(opt_sample)
    {
      sample_writer_.start(
        marker,
        &encode_handler_t::flush_samples,
        std::move(*opt_sample));
    }
    else
    {
      end_sequence_writer_.start(marker, &encode_handler_t::report_success);
    }
  }

  void report_success(cuti::stack_marker_t& marker)
  {
    result_.submit(marker);
  }
  
private :
  cuti::result_t<void>& result_;
  cuti::logging_context_t const& context_;
  EncoderSettings encoder_settings_;
  std::optional<EncodingSession> encoding_session_;

  cuti::subroutine_t<encode_handler_t,
    cuti::reader_t<SessionParams>> session_params_reader_;
  cuti::subroutine_t<encode_handler_t,
    cuti::writer_t<SampleHeaders>> sample_headers_writer_;

  cuti::subroutine_t<encode_handler_t,
    cuti::begin_sequence_reader_t> begin_sequence_reader_;
  cuti::subroutine_t<encode_handler_t,
    cuti::begin_sequence_writer_t> begin_sequence_writer_;

  cuti::subroutine_t<encode_handler_t,
    cuti::end_sequence_checker_t> end_sequence_checker_;
  cuti::subroutine_t<encode_handler_t,
    cuti::reader_t<x26x_proto::frame_t>> frame_reader_;
  cuti::subroutine_t<encode_handler_t,
    cuti::writer_t<x26x_proto::sample_t>> sample_writer_;
  cuti::subroutine_t<encode_handler_t,
    cuti::end_sequence_writer_t> end_sequence_writer_;
};

} // x26x_es_utils

#endif
