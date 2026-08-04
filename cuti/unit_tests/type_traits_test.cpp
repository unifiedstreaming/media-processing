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

#include <cuti/type_traits.hpp>

#include <cstddef>

namespace { // anonymous

using namespace cuti;

struct foo_t
{ };

struct bar_t
{
  bool operator==(std::nullptr_t) const;
};

static_assert(equality_comparable_v<int, int>);
static_assert(equality_comparable_v<int, short>);
static_assert(equality_comparable_v<void*, void*>);
static_assert(equality_comparable_v<void*, void const*>);
static_assert(!equality_comparable_v<int, void*>);
static_assert(!equality_comparable_v<int, std::nullptr_t>);

static_assert(equality_comparable_v<foo_t*, void*>);
static_assert(equality_comparable_v<foo_t*, std::nullptr_t>);
static_assert(!equality_comparable_v<foo_t, std::nullptr_t>);

static_assert(equality_comparable_v<bar_t*, std::nullptr_t>);
static_assert(equality_comparable_v<bar_t, std::nullptr_t>);

static_assert(!equality_comparable_v<foo_t, bar_t>);
static_assert(!equality_comparable_v<foo_t* , bar_t*>);

} // anonymous

int main()
{
  return 0;
}
