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

#include <cuti/input_list.hpp>

#include <string>
#include <utility>

#undef NDEBUG
#include <cassert>

namespace // anonymous
{

using namespace cuti;

void set_inputs(input_list_t<> const& /* inputs */)
{ }

template<typename... Inputs, typename FirstValue, typename... OtherValues>
void set_inputs(input_list_t<Inputs...> const& inputs,
                FirstValue&& first_value,
                OtherValues&&... other_values)
{
  inputs.first().set(std::forward<FirstValue>(first_value));
  set_inputs(inputs.others(), std::forward<OtherValues>(other_values)...);
}

void test_single_value()
{
  int i = 42;
  set_inputs(make_input_list<int>(i), 4711);
  assert(i == 4711);
}

void test_multiple_values()
{
  bool b = false;
  int i = 42;
  std::string s = "Buster";

  set_inputs(make_input_list<bool, int, std::string>(b, i, s),
    true, 4711, "Charlie");

  assert(b == true);
  assert(i = 4711);
  assert(s == "Charlie");
}

void test_single_lambda()
{
  int i = 42;
  auto lambda = [&](int value) { i = value; };

  set_inputs(make_input_list<int>(lambda), 4711);

  assert(i == 4711);
}

void test_multiple_lambdas()
{
  bool b = false;
  auto blambda = [&](bool value) { b = value; };

  int i = 42;
  auto ilambda = [&](int value) { i = value; };

  std::string s = "Buster";
  auto slambda = [&](std::string value) { s = std::move(value); };

  set_inputs(
    make_input_list<bool, int, std::string>(blambda, ilambda, slambda),
    true, 4711, "Charlie");

  assert(b == true);
  assert(i == 4711);
  assert(s == "Charlie");
}

void test_mixed()
{
  bool b = false;
  auto blambda = [&](bool value) { b = value; };

  int i = 42;

  std::string s = "Buster";
  auto slambda = [&](std::string value) { s = std::move(value); };

  set_inputs(
    make_input_list<bool, int, std::string>(blambda, i, slambda),
    true, 4711, "Charlie");

  assert(b == true);
  assert(i == 4711);
  assert(s == "Charlie");
}

} // anonymous

int main()
{
  test_single_value();
  test_multiple_values();
  test_single_lambda();
  test_multiple_lambdas();
  test_mixed();

  return 0;
}
