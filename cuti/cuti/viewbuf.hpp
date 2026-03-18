/*
 * Copyright (C) 2019-2026 CodeShop B.V.
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

#ifndef CUTI_VIEWBUF_HPP_
#define CUTI_VIEWBUF_HPP_

#include "linkage.h"

#include <cstring>
#include <streambuf>
#include <string_view>

namespace cuti
{

/*
 * An input stream buffer that reads from a character array.
 */
struct CUTI_ABI viewbuf_t : std::streambuf
{
  viewbuf_t(char const* begin, char const* end);

  explicit viewbuf_t(char const *str)
  : viewbuf_t(str, str + std::strlen(str))
  { }

  explicit viewbuf_t(std::string_view sv)
  : viewbuf_t(sv.data(), sv.data() + sv.size())
  { }

  viewbuf_t(viewbuf_t const&) = delete;
  viewbuf_t& operator=(viewbuf_t const&) = delete;

  // begin()/cbegin() return a pointer to the first character not yet
  // consumed.
  char const* begin() const
  { return this->gptr(); }

  char const* cbegin() const
  { return this->gptr(); }

  char const* end() const
  { return this->egptr(); }

  char const* cend() const
  { return this->egptr(); }

protected :
  int_type underflow() override;
};

} // cuti

#endif
