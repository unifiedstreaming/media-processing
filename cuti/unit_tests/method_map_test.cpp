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

#include <cuti/add_handler.hpp>
#include <cuti/method_map.hpp>
#include <cuti/subtract_handler.hpp>

#include <iterator>

#undef NDEBUG
#include <cassert>

namespace // anonymous
{

using namespace cuti;

method_map_t::entry_t const entries[] = {
  { "add", make_method_handler<add_handler_t> },
  { "subtract", make_method_handler<subtract_handler_t> }
};

method_map_t const method_map{std::begin(entries), std::end(entries)};
  
void test_method_map()
{
  assert(method_map.find_entry(identifier_t("a")) == nullptr);
  assert(method_map.find_entry(identifier_t("add")) == &entries[0]);
  assert(method_map.find_entry(identifier_t("divide")) == nullptr);
  assert(method_map.find_entry(identifier_t("multiply")) == nullptr);
  assert(method_map.find_entry(identifier_t("subtract")) == &entries[1]);
  assert(method_map.find_entry(identifier_t("z")) == nullptr);
}
  
} // anonymous

int main(int argc, char* argv[])
{
  test_method_map();

  return 0;
}
