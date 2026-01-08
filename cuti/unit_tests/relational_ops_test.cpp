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

#include <cuti/relational_ops.hpp>

#include <string>
#include <utility>

#undef NDEBUG
#include <cassert>

namespace // anonymous
{

namespace user
{

/*
 * User-defined type with operator==() and operator!=()
 */
struct eq_only_wrapper_t :
  cuti::relational_ops_t<eq_only_wrapper_t, std::string, char const*>
{
  explicit eq_only_wrapper_t(std::string wrapped)
  : wrapped_(std::move(wrapped))
  { }

  /*
   * Define an equal_to() for eq_only_wrapper_t and each of the peer
   * types.
   */
  bool equal_to(eq_only_wrapper_t const& that) const
  { return this->wrapped_ == that.wrapped_; }

  bool equal_to(std::string const& that) const
  { return this->wrapped_ == that; }

  bool equal_to(char const* that) const
  { assert(that != nullptr); return this->wrapped_ == that; }

private :
  std::string wrapped_;
};

/*
 * User-defined type with the full set of relational operators
 */
struct full_wrapper_t :
  cuti::relational_ops_t<full_wrapper_t, std::string, char const*>
{
  explicit full_wrapper_t(std::string wrapped)
  : wrapped_(std::move(wrapped))
  { }

  /*
   * To support the relational ops taking two full_wrapper_t
   * instances, only less_than() and equal_to() are required.
   */
  bool less_than(full_wrapper_t const& that) const
  { return this->wrapped_ < that.wrapped_; }

  bool equal_to(full_wrapper_t const& that) const
  { return this->wrapped_ == that.wrapped_; }

  /*
   * To support the relational ops taking a full_wrapper_t and some
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

template<typename T, typename Peer>
void test_equality_ops()
{
  T val1{"val1"};
  Peer val2{"val2"};

  assert(val1 == val1);
  assert(!(val1 == val2));
  assert(!(val2 == val1));

  assert(!(val1 != val1));
  assert(val1 != val2);
  assert(val2 != val1);
}

template<typename T, typename Peer>
void test_all_ops()
{
  test_equality_ops<T, Peer>();

  T val1{"val1"};
  Peer val2{"val2"};

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
  test_equality_ops<user::eq_only_wrapper_t, user::eq_only_wrapper_t>(); 
  test_equality_ops<user::eq_only_wrapper_t, std::string>(); 
  test_equality_ops<user::eq_only_wrapper_t, char const*>(); 

  test_all_ops<user::full_wrapper_t, user::full_wrapper_t>(); 
  test_all_ops<user::full_wrapper_t, std::string>(); 
  test_all_ops<user::full_wrapper_t, char const*>(); 

  return 0;
}
