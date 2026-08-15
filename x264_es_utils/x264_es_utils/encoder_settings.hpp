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

#ifndef X264_ES_UTILS_ENCODER_SETTINGS_HPP_
#define X264_ES_UTILS_ENCODER_SETTINGS_HPP_

#include <cuti/args_reader.hpp>
#include <cuti/flag.hpp>

#include <string>

namespace x264_es_utils
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

  struct session_threads_t
  {
    session_threads_t(int value) : value_(value) { }
    int value_;
  };

  struct session_lookahead_threads_t
  {
    session_lookahead_threads_t(int value) : value_(value) { }
    int value_;
  };

  static constexpr std::string default_preset() { return {}; }
  static constexpr std::string default_tune() { return {}; }
  static constexpr int default_session_threads() { return 0; }
  static constexpr int default_session_lookahead_threads() { return 0; }

  encoder_settings_t()
  : deterministic_()
  , preset_(default_preset())
  , tune_(default_tune())
  , session_threads_(default_session_threads())
  , session_lookahead_threads_(default_session_lookahead_threads())
  , session_sliced_threads_()
  , session_deterministic_()
  , session_cpu_independent_()
  { }

  cuti::flag_t deterministic_;
  preset_t preset_;
  tune_t tune_;
  session_threads_t session_threads_;
  session_lookahead_threads_t session_lookahead_threads_;
  cuti::flag_t session_sliced_threads_;
  cuti::flag_t session_deterministic_;
  cuti::flag_t session_cpu_independent_;
};

void parse_optval(char const* name, cuti::args_reader_t const& reader,
  char const* in, encoder_settings_t::preset_t& out);

void parse_optval(char const* name, cuti::args_reader_t const& reader,
  char const* in, encoder_settings_t::tune_t& out);

void parse_optval(char const* name, cuti::args_reader_t const& reader,
  char const* in, encoder_settings_t::session_threads_t& out);

void parse_optval(char const* name, cuti::args_reader_t const& reader,
  char const* in, encoder_settings_t::session_lookahead_threads_t& out);

} // x264_es_utils

#endif
