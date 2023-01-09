/*
 * Copyright (C) 2021-2023 CodeShop B.V.
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

#include <cuti/identifier.hpp>

#undef NDEBUG
#include <cassert>

namespace // anonymous
{

using namespace cuti;

void test_is_valid()
{
  assert(!identifier_t{}.is_valid());
  assert(!identifier_t{""}.is_valid());
  assert(!identifier_t{"42"}.is_valid());
  assert(!identifier_t{"\'"}.is_valid());
  assert(!identifier_t{"{"}.is_valid());
  assert(!identifier_t{"@"}.is_valid());
  assert(!identifier_t{"["}.is_valid());
  assert(!identifier_t{"a/"}.is_valid());
  assert(!identifier_t{"a:"}.is_valid());

  assert(identifier_t{"a"}.is_valid());
  assert(identifier_t{"A"}.is_valid());
  assert(identifier_t{"z"}.is_valid());
  assert(identifier_t{"Z"}.is_valid());
  assert(identifier_t{"_"}.is_valid());
  
  assert(identifier_t{"aa"}.is_valid());
  assert(identifier_t{"a42"}.is_valid());
  assert(identifier_t{"zz"}.is_valid());
  assert(identifier_t{"z42"}.is_valid());
  assert(identifier_t{"AA"}.is_valid());
  assert(identifier_t{"A42"}.is_valid());
  assert(identifier_t{"ZZ"}.is_valid());
  assert(identifier_t{"Z42"}.is_valid());
  assert(identifier_t{"__"}.is_valid());
  assert(identifier_t{"_42"}.is_valid());
}

} // anonymous

int main()
{
  test_is_valid();

  return 0;
}
