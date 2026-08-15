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

#ifndef X265_ES_UTILS_ENCODER_SETTINGS_HPP_
#define X265_ES_UTILS_ENCODER_SETTINGS_HPP_

#include <cuti/args_reader.hpp>

#include <string>

namespace x265_es_utils
{

struct encoder_settings_t
{
  struct preset_t
  {
    preset_t(std::string value) : value_(std::move(value)) { }
    std::string value_;
  };

  struct tune_t
  {
    tune_t(std::string value) : value_(std::move(value)) { }
    std::string value_;
  };

  struct frame_threads_t
  {
    frame_threads_t(unsigned int value) : value_(value) { }
    unsigned int value_;
  };

  struct numa_pools_t
  {
    numa_pools_t(std::string value) : value_(std::move(value)) { }
    std::string value_;
  };

  static constexpr std::string default_preset() { return {}; }
  static constexpr std::string default_tune() { return {}; }
  static constexpr unsigned int default_frame_threads() { return 0; }
  static constexpr std::string default_numa_pools() { return {}; }

  encoder_settings_t()
  : preset_(default_preset())
  , tune_(default_tune())
  , frame_threads_(default_frame_threads())
  , numa_pools_(default_numa_pools())
  { }

  preset_t preset_;
  tune_t tune_;
  frame_threads_t frame_threads_;
  numa_pools_t numa_pools_;
};

void parse_optval(char const* name, cuti::args_reader_t const& reader,
  char const* in, encoder_settings_t::preset_t& out);

void parse_optval(char const* name, cuti::args_reader_t const& reader,
  char const* in, encoder_settings_t::tune_t& out);

void parse_optval(char const* name, cuti::args_reader_t const& reader,
  char const* in, encoder_settings_t::frame_threads_t& out);

void parse_optval(char const* name, cuti::args_reader_t const& reader,
  char const* in, encoder_settings_t::numa_pools_t& out);

} // x265_es_utils

#endif
