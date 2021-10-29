/*
 * Copyright (C) 2021 CodeShop B.V.
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

#ifndef CUTI_QUOTED_STRING_HPP_
#define CUTI_QUOTED_STRING_HPP_

#include "linkage.h"

#include <cassert>
#include <cstring>
#include <ostream>
#include <streambuf>
#include <string>
#include <utility>
#include <vector>

namespace cuti
{

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
quoted_string_t quoted_string(char const* str)
{
  assert(str != nullptr);
  return quoted_string_t(str, str + std::strlen(str));
}

inline CUTI_ABI
quoted_string_t quoted_string(std::string const& str)
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
