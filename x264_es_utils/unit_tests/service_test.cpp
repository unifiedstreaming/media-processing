/*
 * Copyright (C) 2022-2024 CodeShop B.V.
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

#include <cuti/cmdline_reader.hpp>
#include <cuti/dispatcher.hpp>
#include <cuti/endpoint.hpp>
#include <cuti/flag.hpp>
#include <cuti/logger.hpp>
#include <cuti/logging_context.hpp>
#include <cuti/option_walker.hpp>
#include <cuti/resolver.hpp>
#include <cuti/rpc_client.hpp>
#include <cuti/scoped_guard.hpp>
#include <cuti/scoped_thread.hpp>
#include <cuti/simple_nb_client_cache.hpp>
#include <cuti/streambuf_backend.hpp>
#include <x264_proto/client.hpp>

#include <x264_es_utils/service.hpp>

#include "common.hpp"

#include <csignal>
#include <exception>
#include <iostream>

#undef NDEBUG
#include <cassert>

namespace // anonymous
{

void test_add(cuti::logging_context_t const& context,
              x264_proto::client_t& client)
{
  if(auto msg = context.message_at(cuti::loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  int result = client.add(42, 4711);
  assert(result == 4753);

  if(auto msg = context.message_at(cuti::loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}

void test_subtract(cuti::logging_context_t const& context,
                   x264_proto::client_t& client)
{
  if(auto msg = context.message_at(cuti::loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  int result = client.subtract(4753, 42);
  assert(result == 4711);

  if(auto msg = context.message_at(cuti::loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}

void test_echo(cuti::logging_context_t const& context,
               x264_proto::client_t& client)
{
  if(auto msg = context.message_at(cuti::loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  std::vector<std::string> input{ "Fred", "Jim", "Sheila" };
  auto output = client.echo(input);
  assert(output == input);

  if(auto msg = context.message_at(cuti::loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}

void test_encode(cuti::logging_context_t const& context,
                 x264_proto::client_t& client,
                 std::size_t count)
{
  if(auto msg = context.message_at(cuti::loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  constexpr uint32_t timescale = 600;
  constexpr uint32_t bitrate = 400000;
  constexpr uint32_t width = 640;
  constexpr uint32_t height = 480;
  constexpr auto format = x264_proto::format_t::YUV420P;
  auto session_params = common::make_test_session_params(
    timescale, bitrate, width, height, format);

  constexpr size_t gop_size = 12;
  constexpr uint32_t duration = 25;
  auto frames = common::make_test_frames(count, gop_size,
    width, height, format, timescale, duration, common::black_8);

  auto [sample_headers, samples] = client.encode(
    std::move(session_params), std::move(frames));
  assert(samples.size() == count);

  if(auto msg = context.message_at(cuti::loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}

void test_streaming_encode(cuti::logging_context_t const& context,
                           x264_proto::client_t& client,
                           std::size_t count)
{
  if(auto msg = context.message_at(cuti::loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  x264_proto::sample_headers_t sample_headers;
  std::vector<x264_proto::sample_t> samples;

  constexpr uint32_t timescale = 600;
  constexpr uint32_t bitrate = 400000;
  constexpr uint32_t width = 640;
  constexpr uint32_t height = 480;
  constexpr auto format = x264_proto::format_t::NV12;
  auto session_params = common::make_test_session_params(
    timescale, bitrate, width, height, format);

  constexpr size_t gop_size = 12;
  constexpr uint32_t duration = 25;
  auto frames = common::make_test_frames(count, gop_size,
    width, height, format, timescale, duration, common::black_8);
  
  bool sample_headers_received = false;
  auto sample_headers_consumer = [&](x264_proto::sample_headers_t headers)
  {
    assert(!sample_headers_received);
    if(auto msg = context.message_at(cuti::loglevel_t::info))
    {
      *msg << "received sample headers";
    }
    sample_headers = std::move(headers);
    sample_headers_received = true;
  };

  auto samples_consumer = [&](std::optional<x264_proto::sample_t> opt_sample)
  {
    assert(sample_headers_received);
    if(opt_sample != std::nullopt)
    {
      if(auto msg = context.message_at(cuti::loglevel_t::info))
      {
        *msg << "received sample #" << samples.size();
      }
      samples.push_back(std::move(*opt_sample));
    }
    else
    {
      if(auto msg = context.message_at(cuti::loglevel_t::info))
      {
        *msg << samples.size() << " samples received";
      }
    }
  };

  auto session_params_producer = [&]
  {
    if(auto msg = context.message_at(cuti::loglevel_t::info))
    {
      *msg << "sending session params";
    }
    return std::move(session_params);
  };

  std::size_t frame_index = 0;
  auto frames_producer = [&]
  {
    std::optional<x264_proto::frame_t> result = std::nullopt;
    if(frame_index != frames.size())
    {
      if(auto msg = context.message_at(cuti::loglevel_t::info))
      {
        *msg << "sending frame #" << frame_index;
      }
      result = std::move(frames[frame_index]);
      ++frame_index;
    }
    else
    {
      if(auto msg = context.message_at(cuti::loglevel_t::info))
      {
        *msg << frame_index << " frames sent";
      }
    }
    return result;
  };
        
  client.start_encode(sample_headers_consumer, samples_consumer,
    session_params_producer, frames_producer);
  client.complete_current_call();
  
  assert(samples.size() == count);

  if(auto msg = context.message_at(cuti::loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}

void test_service(cuti::logging_context_t const& client_context,
                  cuti::logging_context_t const& server_context,
                  std::size_t frame_count)
{
  if(auto msg = client_context.message_at(cuti::loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  cuti::dispatcher_config_t dispatcher_config;

  x264_es_utils::encoder_settings_t encoder_settings;
  encoder_settings.deterministic_ = true;

  auto interfaces = cuti::local_interfaces(cuti::any_port);

  {
    x264_es_utils::service_t service(
      server_context, dispatcher_config, encoder_settings, interfaces);

    cuti::scoped_thread_t server_thread([&] { service.run(); });
    cuti::scoped_guard_t stop_guard([&] { service.stop(SIGINT); });

    auto const& endpoints = service.endpoints();
    assert(!endpoints.empty());

    cuti::simple_nb_client_cache_t cache;
    x264_proto::client_t client(client_context, cache, endpoints.front());

    test_add(client_context, client);
    test_subtract(client_context, client);
    test_echo(client_context, client);
    test_encode(client_context, client, frame_count);
    test_streaming_encode(client_context, client, frame_count);
  }

  if(auto msg = client_context.message_at(cuti::loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}

struct options_t
{
  static cuti::loglevel_t constexpr default_loglevel =
    cuti::loglevel_t::error;
  static std::size_t constexpr default_frame_count = 42;

  options_t()
  : enable_server_logging_(false)
  , frame_count_(default_frame_count)
  , loglevel_(default_loglevel)
  { }

  cuti::flag_t enable_server_logging_;
  std::size_t frame_count_;
  cuti::loglevel_t loglevel_;
};

void print_usage(std::ostream& os, char const* argv0)
{
  os << "usage: " << argv0 << " [<option> ...]\n";
  os << "options are:\n";
  os << "  --enable-server-logging  enable server-side logging\n";
  os << "  --frame-count <count>    set frame count " <<
    "(default: " << options_t::default_frame_count << ")\n";
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
         "--enable-server-logging", options.enable_server_logging_) &&
       !walker.match(
         "--frame-count", options.frame_count_) &&
       !walker.match(
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
  cuti::logger_t null_logger(nullptr);
  cuti::logging_context_t client_context(cerr_logger, options.loglevel_);
  cuti::logging_context_t server_context(
    options.enable_server_logging_ ? cerr_logger : null_logger,
    options.loglevel_);

  test_service(client_context, server_context, options.frame_count_);

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
