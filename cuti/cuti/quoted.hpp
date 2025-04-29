/*
 * Copyright (C) 2021-2025 CodeShop B.V.
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

#ifndef CUTI_QUOTED_HPP_
#define CUTI_QUOTED_HPP_

#include "charclass.hpp"
#include "linkage.h"

#include <cassert>
#include <cstring>
#include <ostream>
#include <streambuf>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace cuti
{

struct CUTI_ABI quoted_char_t
{
  explicit quoted_char_t(char c)
  : c_(std::char_traits<char>::to_int_type(c))
  { }

  explicit quoted_char_t(unsigned char c)
  : c_(c)
  { }

  explicit quoted_char_t(int c)
  : c_(c)
  { }

  void print(std::streambuf& sb) const;

  friend std::ostream& operator<<(std::ostream& os,
                                  quoted_char_t const& q)
  {
    std::streambuf* sb = os.rdbuf();
    assert(sb != nullptr);

    q.print(*sb);
    return os;
  }

private :
  int c_;
};

struct CUTI_ABI quoted_string_t
{
  quoted_string_t(char const* first, char const* last)
  : first_(first)
  , last_(last)
  { }

  void print(std::streambuf& sb) const &&;

  friend std::ostream& operator<<(std::ostream& os,
                                  quoted_string_t const&& q)
  {
    std::streambuf* sb = os.rdbuf();
    assert(sb != nullptr);

    std::move(q).print(*sb);
    return os;
  }

private :
  char const* first_;
  char const* last_;
};

inline CUTI_ABI
quoted_char_t quoted_char(char c)
{
  return quoted_char_t(c);
}

inline CUTI_ABI
quoted_char_t quoted_char(unsigned char c)
{
  return quoted_char_t(c);
}

inline CUTI_ABI
quoted_char_t quoted_char(int c)
{
  return quoted_char_t(c);
}

inline CUTI_ABI
quoted_string_t quoted_string(char const* str)
{
  assert(str != nullptr);
  return quoted_string_t(str, str + std::strlen(str));
}

inline CUTI_ABI
quoted_string_t quoted_string(char const* first, char const* last)
{
  assert(first != nullptr);
  return quoted_string_t(first, last);
}

inline CUTI_ABI
quoted_string_t quoted_string(std::string const& str)
{
  return quoted_string_t(str.data(), str.data() + str.size());
}

inline CUTI_ABI
quoted_string_t quoted_string(std::string_view str)
{
  return quoted_string_t(str.data(), str.data() + str.size());
}

inline CUTI_ABI
quoted_string_t quoted_string(std::vector<char> const& str)
{
  return quoted_string_t(str.data(), str.data() + str.size());
}

} // cuti

#endif
