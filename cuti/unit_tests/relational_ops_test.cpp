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

#include <cuti/relational_ops.hpp>

#include <string>
#include <utility>

#undef NDEBUG
#include <cassert>

namespace // anonymous
{

namespace user
{

struct wrapped_string_t :
  cuti::relational_ops_t<wrapped_string_t, std::string, char const*>
{
  explicit wrapped_string_t(std::string wrapped)
  : wrapped_(std::move(wrapped))
  { }

  /*
   * To support the relational ops taking two wrapped_string_t
   * instances, only less_than() and equal_to() are required.
   */
  bool less_than(wrapped_string_t const& that) const
  { return this->wrapped_ < that.wrapped_; }

  bool equal_to(wrapped_string_t const& that) const
  { return this->wrapped_ == that.wrapped_; }

  /*
   * To support the relational ops taking a wrapped_string_t and some
   * other type, less_than(), equal_to() and greater_than() are
   * required.
   */
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

} // user

template<typename Peer>
void test_relational_ops()
{
  user::wrapped_string_t val1{"val1"};
  Peer val2{"val2"};

  assert(val1 == val1);
  assert(!(val1 == val2));
  assert(!(val2 == val1));

  assert(!(val1 != val1));
  assert(val1 != val2);
  assert(val2 != val1);

  assert(!(val1 < val1));
  assert(val1 < val2);
  assert(!(val2 < val1));

  assert(val1 <= val1);
  assert(val1 <= val2);
  assert(!(val2 <= val1));

  assert(!(val1 > val1));
  assert(!(val1 > val2));
  assert(val2 > val1);

  assert(val1 >= val1);
  assert(!(val1 >= val2));
  assert(val2 >= val1);
}

} // anonymous

int main()
{
  test_relational_ops<user::wrapped_string_t>();
  test_relational_ops<std::string>();
  test_relational_ops<char const*>();

  return 0;
}
