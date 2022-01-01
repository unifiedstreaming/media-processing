/*
 * Copyright (C) 2021-2022 CodeShop B.V.
 *
 * This file is part of the cuti library.
 *
 * The cuti library is free software: you can redistribute it and/or
 * modify it under the terms of version 2.1 of the GNU Lesser General
 * Public License as published by the Free Software Foundation.
 *
 * The cuti library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See version
 * 2.1 of the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of version 2.1 of the GNU Lesser
 * General Public License along with the cuti library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "cmdline_reader.hpp"

#include <cassert>

namespace cuti
{

cmdline_reader_t::cmdline_reader_t(int argc, char const* const argv[])
: args_reader_t()
, argc_(argc)
, argv_(argv)
, idx_(1)
{ }

bool cmdline_reader_t::at_end() const
{
  return idx_ >= argc_;
}

char const* cmdline_reader_t::current_argument() const
{
  assert(!this->at_end());
  return argv_[idx_];
}

std::string cmdline_reader_t::current_origin() const
{
  return "command line";
}

void cmdline_reader_t::advance()
{
  assert(!this->at_end());
  ++idx_;
}

} // cuti
