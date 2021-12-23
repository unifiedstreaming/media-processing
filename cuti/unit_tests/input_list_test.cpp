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

#include <optional>
#include <string>
#include <utility>
#include <vector>

#undef NDEBUG
#include <cassert>

namespace // anonymous
{

using namespace cuti;

/*
 * set_inputs(): forward declarations to allow for mutual recursion
 */
void set_inputs(input_list_t<> const& inputs);
  
template<typename... Inputs,
         typename FirstValue, typename... OtherValues>
void set_inputs(
  input_list_t<Inputs...> const& inputs,
  FirstValue&& first_value,
  OtherValues&&... other_values);

template<typename Element, typename... OtherInputs,
         typename Container, typename... OtherValues>
void set_inputs(
  input_list_t<streaming_tag_t<Element>, OtherInputs...> const& inputs,
  Container container,
  OtherValues&&... other_values);

/*
 * set_inputs(): forward declared definitions
 */
void set_inputs(input_list_t<> const& /* inputs */)
{ }
  
template<typename... Inputs,
         typename FirstValue, typename... OtherValues>
void set_inputs(
  input_list_t<Inputs...> const& inputs,
  FirstValue&& first_value,
  OtherValues&&... other_values)
{
  inputs.first().set(std::forward<FirstValue>(first_value));
  set_inputs(inputs.others(), std::forward<OtherValues>(other_values)...);
}

template<typename Element, typename... OtherInputs,
         typename Container, typename... OtherValues>
void set_inputs(
  input_list_t<streaming_tag_t<Element>, OtherInputs...> const& inputs,
  Container container,
  OtherValues&&... other_values)
{
  auto const& first_input = inputs.first();
  for(auto& element : container)
  {
    first_input.set(std::make_optional<Element>(std::move(element)));
  }
  first_input.set(std::nullopt);
  
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

void test_streaming_tag()
{
  std::vector<int> src_vector = {17, 42, 4711};
  std::vector<int> dst_vector;
  bool at_end_stream = false;

  auto lambda = [&](std::optional<int> opt_value)
  {
    assert(!at_end_stream);
    if(opt_value)
    {
      dst_vector.push_back(*opt_value);
    }
    else
    {
      at_end_stream = true;
    }
  };

  set_inputs(make_input_list<streaming_tag_t<int>>(lambda), src_vector);

  assert(dst_vector == src_vector);
  assert(at_end_stream);
}
  

void test_mixed()
{
  bool b = false;
  auto blambda = [&](bool value) { b = value; };

  int i = 42;

  std::vector<int> src_vector = {17, 42, 4711};
  std::vector<int> dst_vector;
  bool at_end_stream = false;

  auto vlambda = [&](std::optional<int> opt_value)
  {
    assert(!at_end_stream);
    if(opt_value)
    {
      dst_vector.push_back(*opt_value);
    }
    else
    {
      at_end_stream = true;
    }
  };

  std::string s = "Buster";
  auto slambda = [&](std::string value) { s = std::move(value); };

  set_inputs(
    make_input_list<bool, int, streaming_tag_t<int>, std::string>(
      blambda, i, vlambda, slambda),
    true, 4711, src_vector, "Charlie");

  assert(b == true);
  assert(i == 4711);
  assert(dst_vector == src_vector);
  assert(at_end_stream);
  assert(s == "Charlie");
}

} // anonymous

int main()
{
  test_single_value();
  test_multiple_values();
  test_single_lambda();
  test_multiple_lambdas();
  test_streaming_tag();
  test_mixed();

  return 0;
}
