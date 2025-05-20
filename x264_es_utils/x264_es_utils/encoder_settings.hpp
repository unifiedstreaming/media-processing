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

#ifndef X264_ES_UTILS_ENCODER_SETTINGS_HPP_
#define X264_ES_UTILS_ENCODER_SETTINGS_HPP_

#include <cuti/flag.hpp>

#include <string>

namespace x264_es_utils
{

struct encoder_settings_t
{
  static unsigned int constexpr default_session_threads()
  { return 0; }

  static unsigned int constexpr default_session_lookahead_threads()
  { return 0; }

  encoder_settings_t()
  : deterministic_()
  , preset_()
  , session_threads_(default_session_threads())
  , session_lookahead_threads_(default_session_lookahead_threads())
  , session_sliced_threads_()
  , session_deterministic_()
  , session_cpu_independent_()
  , tune_()
  { }

  cuti::flag_t deterministic_;
  std::string preset_;
  unsigned int session_threads_;
  unsigned int session_lookahead_threads_;
  cuti::flag_t session_sliced_threads_;
  cuti::flag_t session_deterministic_;
  cuti::flag_t session_cpu_independent_;

  std::string tune_;
};

} // x264_es_utils

#endif
