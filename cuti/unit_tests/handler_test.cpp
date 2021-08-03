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

#include <cuti/handler.hpp>

#include <exception>
#include <iostream>
#include <utility>
#include <type_traits>

// Enable assert
#undef NDEBUG
#include <cassert>

namespace // anonymous
{

using namespace cuti;

struct functor_t
{
  functor_t(bool& called)
  : called_(called)
  { }

  void operator()() const
  {
    called_ = true;
  }

private :
  bool& called_;
};

static_assert(std::is_nothrow_default_constructible_v<handler_t>);
static_assert(std::is_nothrow_copy_constructible_v<handler_t>);
static_assert(std::is_nothrow_move_constructible_v<handler_t>);
static_assert(std::is_nothrow_copy_assignable_v<handler_t>);
static_assert(std::is_nothrow_move_assignable_v<handler_t>);
static_assert(std::is_nothrow_swappable_v<handler_t>);

// check that handler's templated constructor and assignment operator
// (which allocate an impl object) are not noexcept
static_assert(!std::is_nothrow_constructible_v<handler_t, functor_t const&>);
static_assert(!std::is_nothrow_assignable_v<handler_t, functor_t const&>);
static_assert(!std::is_nothrow_constructible_v<handler_t, functor_t&>);
static_assert(!std::is_nothrow_assignable_v<handler_t, functor_t&>);
static_assert(!std::is_nothrow_constructible_v<handler_t, functor_t&&>);
static_assert(!std::is_nothrow_assignable_v<handler_t, functor_t&&>);

// check that handler_t's templated constructor and assignment operator
// are properly SFINEA'd out for handler_t lvalues
static_assert(std::is_nothrow_constructible_v<handler_t, handler_t&>);
static_assert(std::is_nothrow_assignable_v<handler_t, handler_t&>);

// check that handler_t's nullptr overloads don't trigger the templated
// constructor and assignment operator
static_assert(
  std::is_nothrow_constructible_v<handler_t, std::nullptr_t const&>);
static_assert(
  std::is_nothrow_assignable_v<handler_t, std::nullptr_t const&>);
static_assert(
  std::is_nothrow_constructible_v<handler_t, std::nullptr_t&>);
static_assert(
  std::is_nothrow_assignable_v<handler_t, std::nullptr_t&>);
static_assert(
  std::is_nothrow_constructible_v<handler_t, std::nullptr_t&&>);
static_assert(
  std::is_nothrow_assignable_v<handler_t, std::nullptr_t&&>);

bool function_called = false;

void function()
{
  function_called = true;
}

void empty_handler()
{
  handler_t hnd;
  assert(hnd == nullptr);
}

void function_handler()
{
  handler_t hnd(function);
  assert(hnd != nullptr);

  function_called = false;
  hnd();
  assert(function_called);
}

void function_ptr_handler()
{
  void (*f)(void) = nullptr;
  handler_t hnd1(f);
  assert(hnd1 == nullptr);

  f = function;
  handler_t hnd2(f);
  assert(hnd2 != nullptr);
  function_called = false;
  hnd2();
  assert(function_called);

  f = nullptr;
  hnd1 = f;
  assert(hnd1 == nullptr);
  
  f = function;
  hnd2 = f;
  assert(hnd2 != nullptr);
  function_called = false;
  hnd2();
  assert(function_called);
}  
  
void functor_handler()
{
  bool called = false;
  handler_t hnd = functor_t(called);
  assert(hnd != nullptr);
  hnd();
  assert(called);
}

void lambda_handler()
{
  bool called = false;
  handler_t hnd([&] { called = true; });
  assert(hnd != nullptr);
  hnd();
  assert(called);
}

void copy_construct()
{
  handler_t hnd1(function);
  assert(hnd1 != nullptr);

  handler_t hnd2(hnd1);
  assert(hnd1 != nullptr);
  assert(hnd2 != nullptr);
}

void move_construct()
{
  handler_t hnd1(function);
  assert(hnd1 != nullptr);

  handler_t hnd2(std::move(hnd1));
  assert(hnd1 == nullptr);
  assert(hnd2 != nullptr);
}

void copy_assign()
{
  handler_t hnd1(function);
  assert(hnd1 != nullptr);

  handler_t hnd2;
  hnd2 = hnd1;
  assert(hnd1 != nullptr);
  assert(hnd2 != nullptr);
}

void move_assign()
{
  handler_t hnd1(function);
  assert(hnd1 != nullptr);

  handler_t hnd2;
  hnd2 = std::move(hnd1);
  assert(hnd1 == nullptr);
  assert(hnd2 != nullptr);
}

void swapped()
{
  handler_t hnd1(function);
  assert(hnd1 != nullptr);

  handler_t hnd2;
  assert(hnd2 == nullptr);

  swap(hnd1, hnd2);
  assert(hnd1 == nullptr);
  assert(hnd2 != nullptr);
}

void run_tests(int, char const* const*)
{
  empty_handler();
  function_handler();
  function_ptr_handler();
  functor_handler();
  lambda_handler();

  copy_construct();
  move_construct();
  copy_assign();
  move_assign();
  swapped();
}  

} // anonymous

int main(int argc, char* argv[])
{
  try
  {
    run_tests(argc, argv);
  }
  catch(std::exception const& ex)
  {
    std::cerr << argv[0] << ": exception: " << ex.what() << std::endl;
    throw;
  }

  return 0;
}
