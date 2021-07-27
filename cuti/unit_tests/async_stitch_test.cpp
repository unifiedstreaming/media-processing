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

#include <cuti/async_stitch.hpp>

#include <cuti/async_result.hpp>

#include <cstring>
#include <stdexcept>

// Enable assert()
#undef NDEBUG
#include <cassert>

namespace // anonymous
{

using namespace cuti;

// Some async function objects

struct add_t
{
  template<typename Cont, typename T1, typename T2, typename... Args>
  void operator()(Cont&& cont, T1 t1, T2 t2, Args&&... args) const
  {
    cont.submit(t1 + t2, std::forward<Args>(args)...);
  }
};

auto inline constexpr add = add_t{};
 
struct subtract_t
{
  template<typename Cont, typename T1, typename T2, typename... Args>
  void operator()(Cont&& cont, T1 t1, T2 t2, Args&&... args) const
  {
    cont.submit(t1 - t2, std::forward<Args>(args)...);
  }
};

auto inline constexpr subtract = subtract_t{};
 
struct multiply_t
{
  template<typename Cont, typename T1, typename T2, typename... Args>
  void operator()(Cont&& cont, T1 t1, T2 t2, Args&&... args) const
  {
    cont.submit(t1 * t2, std::forward<Args>(args)...);
  }
};

auto inline constexpr multiply = multiply_t{};
 
struct divide_t
{
  template<typename Cont, typename T1, typename T2, typename... Args>
  void operator()(Cont&& cont, T1 t1, T2 t2, Args&&... args) const
  {
    if(t2 == 0)
    {
      cont.fail(std::make_exception_ptr(
        std::runtime_error("division by zero")));
      return;
    }
    cont.submit(t1 / t2, std::forward<Args>(args)...);
  }
};

auto inline constexpr divide = divide_t{};

void test_add()
{
  async_result_t<int> result;
  add(async_result_ref(result), 1, 2);
  assert(result.value() == 3);
}
 
void test_subtract()
{
  async_result_t<int> result;
  subtract(async_result_ref(result), 3, 2);
  assert(result.value() == 1);
}
 
void test_multiply()
{
  async_result_t<int> result;
  multiply(async_result_ref(result), 6, 7);
  assert(result.value() == 42);
}
 
void test_divide()
{
  {
    async_result_t<int> result;
    divide(async_result_ref(result), 42, 7);
    assert(result.value() == 6);
  }

  {
    async_result_t<int> result;
    divide(async_result_ref(result), 42, 0);

    bool caught = false;
    try
    {
      result.value();
    }
    catch(std::exception const& ex)
    {
      caught = true;
      assert(std::strcmp(ex.what(), "division by zero") == 0);
    }
    assert(caught);
  }
}

void test_successful_stitch()
{
  auto stitched = async_stitch(add, divide, subtract, multiply);
  async_result_t<int> result;
  stitched(async_result_ref(result), 9, 7, 2, 1, 6);
  assert(result.value() == 42);
}
  
void test_failing_stitch()
{
  auto stitched = async_stitch(add, divide, subtract, multiply);
  async_result_t<int> result;
  stitched(async_result_ref(result), 9, 7, 0, 1, 6);

  bool caught = false;
  try
  {
    result.value();
  }
  catch(std::exception const& ex)
  {
    caught = true;
    assert(std::strcmp(ex.what(), "division by zero") == 0);
  }
  assert(caught);
}
  
} // anonymous

int main()
{
  test_add();
  test_subtract();
  test_multiply();
  test_divide();

  test_successful_stitch();
  test_failing_stitch();

  return 0;
}
