/*
 * Copyright (C) 2023-2025 CodeShop B.V.
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

#include <cuti/cmdline_reader.hpp>
#include <cuti/io_test_utils.hpp>
#include <cuti/logging_context.hpp>
#include <cuti/option_walker.hpp>
#include <cuti/streambuf_backend.hpp>

#include <x264_proto/types.hpp>

#include <iostream>

#undef NDEBUG
#include <cassert>

namespace // anonymous
{

using namespace x264_proto;

session_params_t make_example_session_params()
{
  session_params_t params;
  params.timescale_ = 25;
  params.bitrate_ = 1000000;
  params.width_ = 1280;
  params.height_ = 720;
  params.sar_width_ = 1;
  params.sar_height_ = 1;
  params.level_idc_ = 30;
  return params;
}

frame_t make_example_frame()
{
  frame_t frame;
  frame.width_ = 1280;
  frame.height_ = 720;
  frame.pts_ = 1000;
  frame.timescale_ = 25;
  frame.data_.insert(frame.data_.begin(), 200, 42);
  return frame;
}

sample_headers_t make_example_sample_headers()
{
  sample_headers_t headers;
  headers.sps_.insert(headers.sps_.begin(), 32, 43);
  headers.pps_.insert(headers.pps_.begin(), 16, 44);
  return headers;
}

sample_t make_example_sample()
{
  sample_t sample;
  sample.dts_ = 1000;
  sample.pts_ = 1100;
  sample.type_ = sample_t::type_t::b;
  sample.data_.insert(sample.data_.begin(), 200, 45);
  return sample;
}

void test_serialization(
  cuti::logging_context_t const& context,
  std::size_t bufsize)
{
  using cuti::io_test_utils::test_roundtrip;

  test_roundtrip(context, bufsize, make_example_session_params());
  test_roundtrip(context, bufsize, make_example_frame());
  test_roundtrip(context, bufsize, make_example_sample_headers());
  test_roundtrip(context, bufsize, make_example_sample());
}

struct options_t
{
  constexpr static cuti::loglevel_t default_loglevel = cuti::loglevel_t::error;

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
    "(default: " << loglevel_string(options_t::default_loglevel) << ")\n";
  os << std::flush;
}

void read_options(options_t& options, cuti::option_walker_t& walker)
{
  while(!walker.done())
  {
    if(!walker.match("--loglevel", options.loglevel_))
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

  cuti::logger_t logger(
    std::make_unique<cuti::streambuf_backend_t>(std::cerr));
  cuti::logging_context_t context(logger, options.loglevel_);

  test_serialization(context, 1);
  test_serialization(context, cuti::nb_outbuf_t::default_bufsize);

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
