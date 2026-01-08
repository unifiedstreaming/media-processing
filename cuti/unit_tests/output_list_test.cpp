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

#include <cuti/output_list.hpp>

#include <optional>
#include <string>
#include <utility>
#include <vector>

#undef NDEBUG
#include <cassert>

namespace // anonymous
{

using namespace cuti;

template<typename T, typename Value>
void check_single_output(output_t<T>& output, Value const& value)
{
  assert(output.get() == value);
}

template<typename T, typename Values>
void check_single_output(output_t<sequence_t<T>>& output,
                         Values const& values)
{
  for(auto const& value : values)
  {
    auto opt_produced = output.get();
    assert(opt_produced != std::nullopt);
    assert(*opt_produced == value);
  }
  assert(output.get() == std::nullopt);
}

void check_outputs(output_list_t<>& /* outputs */)
{ }

template<typename... Outputs,
         typename FirstValue, typename... OtherValues>
void check_outputs(output_list_t<Outputs...>& outputs,
                   FirstValue const& first_value,
                   OtherValues const&... other_values)
{
  check_single_output(outputs.first(), first_value);
  check_outputs(outputs.others(), other_values...);
}

void test_single_value()
{
  auto outputs = make_output_list<int>(42);
  check_outputs(outputs, 42);
}

void test_multiple_values()
{
  auto outputs = make_output_list<bool, int, std::string>(
    true, 4711, "Charlie");
  check_outputs(outputs, true, 4711, "Charlie");
}

void test_single_lambda()
{
  int i = 4711;
  auto lambda = [&] { return i; };

  auto outputs = make_output_list<int>(lambda);
  check_outputs(outputs, 4711);
}

void test_multiple_lambdas()
{
  bool b = true;
  auto blambda = [&] { return b; };

  int i = 4711;
  auto ilambda = [&] { return i; };

  std::string s = "Charlie";
  auto slambda = [&] { return std::move(s); };

  auto outputs = make_output_list<bool, int, std::string>(
    blambda, ilambda, slambda);
  check_outputs(outputs, true, 4711, "Charlie");
}

void test_streaming_vector()
{
  std::vector<int> const vect = {17, 42, 4711};

  auto outputs = make_output_list<sequence_t<int>>(vect);
  check_outputs(outputs, vect);
}

void test_streaming_lambda()
{
  std::vector<int> const vect = {17, 42, 4711};
  auto lambda = [ first = vect.begin(), last = vect.end() ] () mutable
  {
    return first != last ? std::make_optional(*first++) : std::nullopt;
  };

  auto outputs = make_output_list<sequence_t<int>>(lambda);
  check_outputs(outputs, vect);
}

void test_mixed()
{
  bool b = true;
  auto blambda = [&] { return b; };

  std::vector<int> const vect = {17, 42, 4711};
  auto vlambda = [ first = vect.begin(), last = vect.end() ] () mutable
  {
    return first != last ? std::make_optional(*first++) : std::nullopt;
  };

  std::string s = "Charlie";
  auto slambda = [&] { return std::move(s); };

  auto outputs =
    make_output_list<
      bool, int, sequence_t<int>, sequence_t<int>, std::string>(
      blambda, 42, vect, vlambda, slambda);
  check_outputs(outputs, true, 42, vect, vect, "Charlie");
}
  
} // anonymous

int main()
{
  test_single_value();
  test_multiple_values();
  test_single_lambda();
  test_multiple_lambdas();
  test_streaming_vector();
  test_streaming_lambda();
  test_mixed();

  return 0;
}
