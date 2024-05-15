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

#include <x264_es_utils/encoding_session.hpp>

#include <cuti/cmdline_reader.hpp>
#include <cuti/logger.hpp>
#include <cuti/logging_context.hpp>
#include <cuti/option_walker.hpp>
#include <cuti/scoped_thread.hpp>
#include <cuti/streambuf_backend.hpp>

#include <x264_proto/client.hpp>

#include "fnv1a32_hash.hpp"

#include <csignal>
#include <exception>
#include <iostream>

#undef NDEBUG
#include <cassert>

namespace // anonymous
{

x264_proto::session_params_t make_test_session_params(
  uint32_t timescale, uint32_t bitrate,
  uint32_t width, uint32_t height)
{
  x264_proto::session_params_t session_params;

  session_params.timescale_ = timescale;
  session_params.bitrate_ = bitrate;
  session_params.width_ = width;
  session_params.height_ = height;

  return session_params;
}

constexpr uint8_t black_y = 0x10;
constexpr uint8_t black_u = 0x80;
constexpr uint8_t black_v = 0x80;

std::vector<uint8_t> make_test_frame_data(
  uint32_t width, uint32_t height,
  uint8_t y, uint8_t u, uint8_t v)
{
  std::vector<uint8_t> data;

  uint32_t size_y = width * height;
  uint32_t size_uv = width * height / 2;

  data.insert(data.end(), size_y, y);
  if(u == v)
  {
    data.insert(data.end(), size_uv, u);
  }
  else
  {
    for(uint32_t i = 0; i < size_uv; i +=2)
    {
      data.insert(data.end(), u);
      data.insert(data.end(), v);
    }
  }

  return data;
}

x264_proto::frame_t make_test_frame(
  uint32_t width, uint32_t height,
  uint64_t pts, uint32_t timescale,
  bool keyframe,
  uint8_t y, uint8_t u, uint8_t v)
{
  x264_proto::frame_t frame;

  frame.width_ = width;
  frame.height_ = height;
  frame.pts_ = pts;
  frame.timescale_ = timescale;
  frame.keyframe_ = keyframe;
  frame.data_ = make_test_frame_data(width, height, y, u, v);

  return frame;
}

std::vector<x264_proto::frame_t> make_test_frames(
  size_t count, size_t gop_size,
  uint32_t width, uint32_t height,
  uint32_t timescale, uint32_t duration,
  uint8_t y, uint8_t u, uint8_t v)
{
  std::vector<x264_proto::frame_t> frames;

  uint64_t pts = 0;
  for(size_t i = 0; i < count; ++i, pts += duration)
  {
    bool keyframe = i % gop_size == 0;
    frames.push_back(
      make_test_frame(width, height, pts, timescale, keyframe, y, u, v));
  }

  return frames;
}

void run_session(cuti::logging_context_t const& context)
{
  x264_es_utils::encoder_settings_t encoder_settings;
  encoder_settings.deterministic_ = true;

  constexpr uint32_t timescale = 600;
  constexpr uint32_t bitrate = 400000;
  constexpr uint32_t width = 640;
  constexpr uint32_t height = 480;
  auto session_params = make_test_session_params(
    timescale, bitrate, width, height);

  constexpr size_t count = 42;
  constexpr size_t gop_size = 12;
  constexpr uint32_t duration = 25;
  auto frames = make_test_frames(count, gop_size,
    width, height, timescale, duration, black_y, black_u, black_v);

  x264_es_utils::encoding_session_t session(
    context, encoder_settings, session_params);

  auto sample_headers = session.sample_headers();
  if(auto msg = context.message_at(cuti::loglevel_t::warning))
  {
    auto hash =
      fnv1a32::hash(sample_headers.sps_.data(), sample_headers.sps_.size());
    *msg << __func__ << ": sps size=" << sample_headers.sps_.size() <<
      " hash=0x" << std::hex << hash;
  }
  if(auto msg = context.message_at(cuti::loglevel_t::warning))
  {
    auto hash =
      fnv1a32::hash(sample_headers.pps_.data(), sample_headers.pps_.size());
    *msg << __func__ << ": pps size=" << sample_headers.pps_.size() <<
      " hash=0x" << std::hex << hash;
  }

  std::vector<x264_proto::sample_t> samples;

  for(auto& frame : frames)
  {
    if(auto opt_sample = session.encode(std::move(frame)))
    {
      samples.push_back(std::move(*opt_sample));
    }
  }

  while(auto opt_sample = session.flush())
  {
    samples.push_back(std::move(*opt_sample));
  }

  assert(samples.size() == count);

  unsigned idx = 0;
  for(auto const& sample : samples)
  {
    if(auto msg = context.message_at(cuti::loglevel_t::warning))
    {
      auto hash = fnv1a32::hash(sample.data_.data(), sample.data_.size());
      *msg << __func__ << ": sample[" << idx << "] size=" <<
        sample.data_.size() << " hash=" << std::hex << hash;
    }
    idx++;
  }
}

void test_session_in_main_thread(cuti::logging_context_t const& context)
{
  if(auto msg = context.message_at(cuti::loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  run_session(context);

  if(auto msg = context.message_at(cuti::loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}

void test_session_in_separate_thread(cuti::logging_context_t const& context)
{
  if(auto msg = context.message_at(cuti::loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  {
    cuti::scoped_thread_t runner([&] { run_session(context); });
  }

  if(auto msg = context.message_at(cuti::loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}

struct options_t
{
  static cuti::loglevel_t constexpr default_loglevel =
    cuti::loglevel_t::warning;

  options_t()
  : loglevel_(default_loglevel)
  { }

  cuti::loglevel_t loglevel_;
};

void print_usage(std::ostream& os, char const* argv0)
{
  os << "usage: " << argv0 << " [<option> ...]\n";
  os << "options are:\n";
  os << "  --loglevel <level>       set loglevel " <<
    "(default: " << cuti::loglevel_string(options_t::default_loglevel) <<
    ")\n";
  os << std::flush;
}

void read_options(options_t& options, cuti::option_walker_t& walker)
{
  while(!walker.done())
  {
    if(!walker.match(
         "--loglevel", options.loglevel_))
    {
      break;
    }
  }
}

int run_tests(int argc, char const* const* argv)
{
  options_t options;
  cuti::cmdline_reader_t reader(argc, argv);
  cuti::option_walker_t walker(reader);

  read_options(options, walker);
  if(!walker.done() || !reader.at_end())
  {
    print_usage(std::cerr, argv[0]);
    return 1;
  }

  cuti::logger_t cerr_logger(
    std::make_unique<cuti::streambuf_backend_t>(std::cerr));
  cuti::logging_context_t context(cerr_logger, options.loglevel_);

  test_session_in_main_thread(context);
  test_session_in_separate_thread(context);

  return 0;
}

} // anonymous

int main(int argc, char* argv[])
{
  try
  {
    return run_tests(argc, argv);
  }
  catch(std::exception const& ex)
  {
    std::cerr << argv[0] << ": exception: " << ex.what() << std::endl;
  }

  return 1;
}
