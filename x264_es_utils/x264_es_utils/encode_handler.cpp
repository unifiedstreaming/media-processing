/*
 * Copyright (C) 2024-2026 CodeShop B.V.
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

#include "encode_handler.hpp"

#include <cassert>
#include <exception>
#include <optional>
#include <utility>

namespace x264_es_utils
{

encode_handler_t::encode_handler_t(cuti::result_t<void>& result,
                                   cuti::logging_context_t const& context,
		                   cuti::bound_inbuf_t& inbuf,
		                   cuti::bound_outbuf_t& outbuf,
		                   encoder_settings_t encoder_settings)
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

void encode_handler_t::start(cuti::stack_marker_t& marker)
{
  session_params_reader_.start(marker, &encode_handler_t::create_session);
}

void encode_handler_t::create_session(
  cuti::stack_marker_t& marker,
  x264_proto::session_params_t session_params)
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

void encode_handler_t::read_begin_sequence(cuti::stack_marker_t& marker)
{
  begin_sequence_reader_.start(
    marker,
    &encode_handler_t::write_begin_sequence);
}

void encode_handler_t::write_begin_sequence(cuti::stack_marker_t& marker)
{
  begin_sequence_writer_.start(marker, &encode_handler_t::check_eos);
}

void encode_handler_t::check_eos(cuti::stack_marker_t& marker)
{
  end_sequence_checker_.start(marker, &encode_handler_t::handle_eos_check);
}

void encode_handler_t::handle_eos_check(
  cuti::stack_marker_t& marker,
  bool at_end)
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

void encode_handler_t::encode_frame(
  cuti::stack_marker_t& marker,
  x264_proto::frame_t frame)
{
  assert(encoding_session_ != std::nullopt);

  std::optional<x264_proto::sample_t> opt_sample;
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

void encode_handler_t::flush_samples(cuti::stack_marker_t& marker)
{
  assert(encoding_session_ != std::nullopt);

  std::optional<x264_proto::sample_t> opt_sample;
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

void encode_handler_t::report_success(cuti::stack_marker_t& marker)
{
  result_.submit(marker);
}

} // x264_es_utils
