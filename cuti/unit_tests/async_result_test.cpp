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

#include <cuti/async_result.hpp>

#include <cstring>
#include <stdexcept>

// Enable assert()
#undef NDEBUG
#include <cassert>

namespace // anonymous
{

using namespace cuti;

void test_void_not_available()
{
  async_result_t<void> result;
  assert(!result.available());
}  

void test_void_success()
{
  async_result_t<void> result;

  auto cont = async_result_ref(result);
  cont.submit();

  assert(result.available());
  assert(result.exception() == nullptr);
  result.value();
}  

void test_void_exception()
{
  async_result_t<void> result;

  auto cont = async_result_ref(result);
  cont.fail(std::make_exception_ptr(std::runtime_error("oops")));

  assert(result.available());
  assert(result.exception() != nullptr);

  bool caught = false;
  try
  {
    result.value();
  }
  catch(std::exception const& ex)
  {
    caught = true;
    assert(std::strcmp(ex.what(), "oops") == 0);
  }
  assert(caught);
}  

void test_typed_not_available()
{
  async_result_t<int> result;
  assert(!result.available());
}  

void test_typed_success()
{
  async_result_t<int> result;

  auto cont = async_result_ref(result);
  cont.submit(42);

  assert(result.available());
  assert(result.exception() == nullptr);
  assert(result.value() == 42);
}  

void test_typed_exception()
{
  async_result_t<int> result;

  auto cont = async_result_ref(result);
  cont.fail(std::make_exception_ptr(std::runtime_error("oops")));

  assert(result.available());
  assert(result.exception() != nullptr);

  bool caught = false;
  try
  {
    result.value();
  }
  catch(std::exception const& ex)
  {
    caught = true;
    assert(std::strcmp(ex.what(), "oops") == 0);
  }
  assert(caught);
}  

} // anonymous

int main()
{
  test_void_not_available();
  test_void_success();
  test_void_exception();

  test_typed_not_available();
  test_typed_success();
  test_typed_exception();

  return 0;
}
