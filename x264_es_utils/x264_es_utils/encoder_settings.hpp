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

#ifndef X264_ES_UTILS_ENCODER_SETTINGS_HPP_
#define X264_ES_UTILS_ENCODER_SETTINGS_HPP_

namespace x264_es_utils
{

struct encoder_settings_t
{
  static bool constexpr default_deterministic = true;
 
  encoder_settings_t()
  : deterministic_(default_deterministic)
  { }

  bool deterministic_;
};

} // x264_es_utils

#endif
