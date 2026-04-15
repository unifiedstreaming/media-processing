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

#include <cuti/alternative_index.hpp>

#include <string>
#include <variant>

namespace { // anonymous

using namespace cuti;

using my_variant = std::variant<char, int, std::string>;

static_assert(has_alternative_v<char, my_variant>);
static_assert(has_alternative_v<int, my_variant>);
static_assert(has_alternative_v<std::string, my_variant>);

static_assert(!has_alternative_v<double, my_variant>);

static_assert(alternative_index_v<char, my_variant> == 0);
static_assert(alternative_index_v<int, my_variant> == 1);
static_assert(alternative_index_v<std::string, my_variant> == 2);

} // anonymous

int main()
{
  return 0;
}
