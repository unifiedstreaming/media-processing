/*
 * Copyright (C) 2021-2026 CodeShop B.V.
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

#ifndef CUTI_IDENTIFIER_HPP_
#define CUTI_IDENTIFIER_HPP_

#include "charclass.hpp"
#include "linkage.h"

#include <algorithm>
#include <cassert>
#include <compare>
#include <ostream>
#include <string>
#include <utility>

namespace cuti
{

/*
 * Value type for C-style identifiers: [A-Za-z_][A-Za-z_0-9]*.
 * Please note: identifier_t instances may not be valid; use
 * is_valid() to check.
 */
struct identifier_t
{
  static constexpr bool is_leader(int c)
  { return is_alpha(c) || c == '_'; }

  static constexpr bool is_follower(int c)
  { return is_leader(c) || is_digit(c); }

  identifier_t()
  : wrapped_()
  { }

  identifier_t(std::string wrapped)
  : wrapped_(std::move(wrapped))
  { }

  identifier_t(char const* wrapped)
  : wrapped_((assert(wrapped != nullptr), wrapped))
  { }

  bool is_valid() const
  {
    auto begin = wrapped_.begin();
    auto end = wrapped_.end();
    return begin != end && is_leader(*begin) &&
      std::find_if_not(begin + 1, end, is_follower) == end;
  }

  std::string const& as_string() const
  { return wrapped_; }

  bool operator==(identifier_t const& that) const
  { return this->wrapped_ == that.wrapped_; }
  
  bool operator==(std::string const& that) const
  { return this->wrapped_ == that; }
  
  bool operator==(char const* that) const
  { assert(that != nullptr); return this->wrapped_ == that; }
  
  auto operator<=>(identifier_t const& that) const
  { return this->wrapped_ <=> that.wrapped_; }
  
  auto operator<=>(std::string const& that) const
  { return this->wrapped_ <=> that; }
  
  auto operator<=>(char const* that) const
  { assert(that != nullptr); return this->wrapped_ <=> that; }
  
  friend CUTI_ABI
  std::ostream& operator<<(std::ostream& os, identifier_t const& value)
  { return os << value.wrapped_; }
  
private :
  std::string wrapped_;
};

} // cuti

#endif
