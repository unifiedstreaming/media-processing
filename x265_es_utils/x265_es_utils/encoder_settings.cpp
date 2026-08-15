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

#include "encoder_settings.hpp"
#include "x265_exception.hpp"

#include <cuti/option_walker.hpp>

#include <cstring>

#include <x265.h>

namespace x265_es_utils
{

namespace // anonymous
{

void parse_optval(char const* name, cuti::args_reader_t const& reader, char const* in, std::string& out, char const* const* choices)
{
  for(auto const* const* choice = choices; *choice != nullptr; ++choice)
  {
    if(std::strcmp(*choice, in) == 0)
    {
      out = *choice;
      return;
    }
  }

  x265_exception_builder_t builder;
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

} // anonymous namespace

void parse_optval(char const* name, cuti::args_reader_t const& reader,
  char const* in, encoder_settings_t::preset_t& out)
{
  parse_optval(name, reader, in, out.value_, x265_preset_names);
}

void parse_optval(char const* name, cuti::args_reader_t const& reader,
  char const* in, encoder_settings_t::tune_t& out)
{
  parse_optval(name, reader, in, out.value_, x265_tune_names);
}

void parse_optval(char const* name, cuti::args_reader_t const& reader,
  char const* in, encoder_settings_t::frame_threads_t& out)
{
  parse_optval(name, reader, in, out.value_);
  if(out.value_ > X265_MAX_FRAME_THREADS)
  {
    x265_exception_builder_t builder;
    builder << reader.current_origin() <<
      ": invalid value '" << in << "' for option '" << name <<
      "'; valid values are 0 through " << X265_MAX_FRAME_THREADS;
    builder.explode();
  }
}

} // x265_es_utils
