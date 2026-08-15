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

#include "encoder_settings.hpp"
#include "x264_exception.hpp"

#include <cuti/option_walker.hpp>

#include <cstdint>
#include <cstring>

#include <x264.h>

namespace x264_es_utils
{

namespace // anonymous
{

void parse_optval(char const* name, cuti::args_reader_t const& reader,
  char const* in, std::string& out, char const* const* choices)
{
  for(auto const* const* choice = choices; *choice != nullptr; ++choice)
  {
    if(std::strcmp(*choice, in) == 0)
    {
      out = *choice;
      return;
    }
  }

  x264_exception_builder_t builder;
  builder << reader.current_origin() <<
    ": invalid value '" << in << "' for option '" << name <<
    "'; valid values are ";
  for(auto const* const* choice = choices; *choice != nullptr; ++choice)
  {
    if(*(choice + 1) == nullptr)
    {
      builder << " and ";
    }
    else if(choice != choices)
    {
      builder << ", ";
    }
    builder << "'" << *choice << "'";
  }
  builder.explode();
}

void parse_optval(char const* name, cuti::args_reader_t const& reader,
  char const* in, int& out, int max_value)
{
  parse_optval(name, reader, in, out);
  if(out < 0 || out > max_value)
  {
    x264_exception_builder_t builder;
    builder << reader.current_origin() <<
      ": invalid value '" << in << "' for option '" << name <<
      "'; valid values are 0 through " << max_value;
    builder.explode();
  }
}

} // anonymous namespace

void parse_optval(char const* name, cuti::args_reader_t const& reader,
  char const* in, encoder_settings_t::preset_t& out)
{
  parse_optval(name, reader, in, out.value_, x264_preset_names);
}

void parse_optval(char const* name, cuti::args_reader_t const& reader,
  char const* in, encoder_settings_t::tune_t& out)
{
  parse_optval(name, reader, in, out.value_, x264_tune_names);
}

void parse_optval(char const* name, cuti::args_reader_t const& reader,
  char const* in, encoder_settings_t::session_threads_t& out)
{
  // defined in x264/common/base.h, but not exposed publicly
  constexpr int X264_THREAD_MAX = 128;
  parse_optval(name, reader, in, out.value_, X264_THREAD_MAX);
}

void parse_optval(char const* name, cuti::args_reader_t const& reader,
  char const* in, encoder_settings_t::session_lookahead_threads_t& out)
{
  // defined in x264/common/base.h, but not exposed publicly
  constexpr int X264_LOOKAHEAD_THREAD_MAX = 16;
  parse_optval(name, reader, in, out.value_, X264_LOOKAHEAD_THREAD_MAX);
}

} // x264_es_utils
