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

#ifndef CUTI_IDENTIFIER_HPP_
#define CUTI_IDENTIFIER_HPP_

#include "charclass.hpp"
#include "linkage.h"
#include "relational_ops.hpp"

#include <algorithm>
#include <cassert>
#include <ostream>
#include <string>
#include <utility>

namespace cuti
{

/*
 * Value type for C-style identifiers: [A-Za-z_][A-Za-z_0-9]*
 * Please note: due to the implicit use of move semantics,
 * identifier_t instances may be in an invalid state. 
 */
struct identifier_t
  : relational_ops_t<identifier_t, std::string, char const*>
{
  static constexpr bool is_leader(int c)
  { return is_alpha(c) || c == '_'; }

  static constexpr bool is_follower(int c)
  { return is_leader(c) || is_digit(c); }

  identifier_t()
  : wrapped_()
  { }

  explicit identifier_t(std::string wrapped)
  : wrapped_(std::move(wrapped))
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

  friend std::ostream& operator<<(std::ostream& os, identifier_t const& value)
  { return os << value.wrapped_; }
  
  /*
   * Required by relational ops_t<identifier_t, std::string, char const*>:
   */
  bool less_than(identifier_t const& that) const
  { return this->wrapped_ < that.wrapped_; }

  bool equal_to(identifier_t const& that) const
  { return this->wrapped_ == that.wrapped_; }

  bool less_than(std::string const& that) const
  { return this->wrapped_ < that; }

  bool equal_to(std::string const& that) const
  { return this->wrapped_ == that; }

  bool greater_than(std::string const& that) const
  { return this->wrapped_ > that; }

  bool less_than(char const* that) const
  { assert(that != nullptr); return this->wrapped_ < that; }

  bool equal_to(char const* that) const
  { assert(that != nullptr); return this->wrapped_ == that; }

  bool greater_than(char const* that) const
  { assert(that != nullptr); return this->wrapped_ > that; }

private :
  std::string wrapped_;
};

} // cuti

#endif
