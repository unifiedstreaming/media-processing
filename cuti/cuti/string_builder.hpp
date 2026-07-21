/*
 * Copyright (C) 2026 CodeShop B.V.
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

#ifndef CUTI_STRING_BUILDER_HPP_
#define CUTI_STRING_BUILDER_HPP_

#include <ostream>
#include <string>

#include "linkage.h"
#include "membuf.hpp"

namespace cuti
{

struct CUTI_ABI string_builder_t : std::ostream
{
  string_builder_t();
  
  string_builder_t(string_builder_t const&) = delete;
  string_builder_t& operator=(string_builder_t const&) = delete;

  char const* begin() const
  { return buf_.begin(); }

  char const* end() const
  { return buf_.end(); }

  std::string result() const
  { return std::string(this->begin(), this->end()); }

  ~string_builder_t() override;

private :
  membuf_t buf_;
};

} // cuti

#endif
