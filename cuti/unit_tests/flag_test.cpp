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

#include <cuti/flag.hpp>

namespace // anonymous
{

using namespace cuti;

constexpr flag_t default_flag;
static_assert(! default_flag);

constexpr flag_t false_flag = false;
static_assert(! false_flag);

constexpr flag_t true_flag = true;
static_assert(true_flag);

static_assert(false_flag == default_flag);
static_assert(false_flag != true_flag);

static_assert(false_flag == false);
static_assert(false == false_flag);

static_assert(true_flag != false);
static_assert(false != true_flag);

} // anonymous

int main()
{
  return 0;
}

