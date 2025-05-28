/*
 * Copyright (C) 2025 CodeShop B.V.
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

#ifndef CUTI_HEXDUMP_HPP_
#define CUTI_HEXDUMP_HPP_

#include "linkage.h"

#include <array>
#include <ostream>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace cuti
{

struct CUTI_ABI hexdump_t
{
  hexdump_t(unsigned char const* first, unsigned char const* last)
  : first_(first)
  , last_(last)
  { }

  void print(std::ostream& os) const &&;

  friend CUTI_ABI
  std::ostream& operator<<(std::ostream& os, hexdump_t const&& dump)
  {
    std::move(dump).print(os);
    return os;
  }

private :
  unsigned char const* first_;
  unsigned char const* last_;
};

inline CUTI_ABI
hexdump_t hexdump(unsigned char const* first, unsigned char const* last)
{
  return hexdump_t(first, last);
}

template<std::size_t N>
hexdump_t hexdump(std::array<unsigned char, N> const& arr)
{
  return hexdump(arr.data(), arr.data() + N);
}

inline CUTI_ABI
hexdump_t hexdump(std::span<unsigned char const> span)
{
  return hexdump(span.data(), span.data() + span.size());
}

inline CUTI_ABI
hexdump_t hexdump(std::vector<unsigned char> const& vec)
{
  return hexdump(vec.data(), vec.data() + vec.size());
}

inline CUTI_ABI
hexdump_t hexdump(char const* first, char const* last)
{
  return hexdump(
    reinterpret_cast<unsigned char const*>(first),
    reinterpret_cast<unsigned char const*>(last)
  );
}

template<std::size_t N>
hexdump_t hexdump(std::array<char, N> const& arr)
{
  return hexdump(arr.data(), arr.data() + N);
}

inline CUTI_ABI
hexdump_t hexdump(std::string const& str)
{
  return hexdump(str.data(), str.data() + str.size());
}

inline CUTI_ABI
hexdump_t hexdump(std::string_view view)
{
  return hexdump(view.data(), view.data() + view.size());
}

inline CUTI_ABI
hexdump_t hexdump(std::vector<char> const& vec)
{
  return hexdump(vec.data(), vec.data() + vec.size());
}

} // cuti

#endif
