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

#include <cuti/callback.hpp>

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

static_assert(std::is_nothrow_default_constructible_v<callback_t>);
static_assert(!std::is_copy_constructible_v<callback_t>);
static_assert(std::is_nothrow_move_constructible_v<callback_t>);
static_assert(!std::is_copy_assignable_v<callback_t>);
static_assert(std::is_nothrow_move_assignable_v<callback_t>);
static_assert(std::is_nothrow_swappable_v<callback_t>);

// check that callback's templated constructor and assignment operator
// (which allocate an impl object) are not noexcept
static_assert(!std::is_nothrow_constructible_v<callback_t, functor_t const&>);
static_assert(!std::is_nothrow_assignable_v<callback_t, functor_t const&>);
static_assert(!std::is_nothrow_constructible_v<callback_t, functor_t&>);
static_assert(!std::is_nothrow_assignable_v<callback_t, functor_t&>);
static_assert(!std::is_nothrow_constructible_v<callback_t, functor_t&&>);
static_assert(!std::is_nothrow_assignable_v<callback_t, functor_t&&>);

// check that callback_t's templated constructor and assignment operator
// are properly SFINEA'd out for callback_t lvalues
static_assert(!std::is_constructible_v<callback_t, callback_t&>);
static_assert(!std::is_assignable_v<callback_t, callback_t&>);

// check that callback_t's nullptr overloads don't trigger the templated
// constructor and assignment operator
static_assert(
  std::is_nothrow_constructible_v<callback_t, std::nullptr_t const&>);
static_assert(
  std::is_nothrow_assignable_v<callback_t, std::nullptr_t const&>);
static_assert(
  std::is_nothrow_constructible_v<callback_t, std::nullptr_t&>);
static_assert(
  std::is_nothrow_assignable_v<callback_t, std::nullptr_t&>);
static_assert(
  std::is_nothrow_constructible_v<callback_t, std::nullptr_t&&>);
static_assert(
  std::is_nothrow_assignable_v<callback_t, std::nullptr_t&&>);

bool function_called = false;

void function()
{
  function_called = true;
}

void empty_callback()
{
  callback_t cb;
  assert(cb == nullptr);
}

void function_callback()
{
  callback_t cb(function);
  assert(cb != nullptr);

  function_called = false;
  cb();
  assert(function_called);
}

void function_ptr_callback()
{
  void (*f)(void) = nullptr;
  callback_t cb1(f);
  assert(cb1 == nullptr);

  f = function;
  callback_t cb2(f);
  assert(cb2 != nullptr);
  function_called = false;
  cb2();
  assert(function_called);

  f = nullptr;
  cb1 = f;
  assert(cb1 == nullptr);
  
  f = function;
  cb2 = f;
  assert(cb2 != nullptr);
  function_called = false;
  cb2();
  assert(function_called);
}  
  
void functor_callback()
{
  bool called = false;
  callback_t cb = functor_t(called);
  assert(cb != nullptr);
  cb();
  assert(called);
}

void lambda_callback()
{
  bool called = false;
  callback_t cb([&] { called = true; });
  assert(cb != nullptr);
  cb();
  assert(called);
}

void mutable_lambda_callback()
{
  bool called_twice = false;
  callback_t cb([&called_twice, cnt = 0]() mutable
  {
    if(++cnt == 2)
    {
      called_twice = true;
    }
  });

  assert(cb != nullptr);
  cb();
  cb();
  assert(called_twice);
}

void move_construct()
{
  callback_t cb1(function);
  assert(cb1 != nullptr);

  callback_t cb2(std::move(cb1));
  assert(cb1 == nullptr);
  assert(cb2 != nullptr);
}

void move_assign()
{
  callback_t cb1(function);
  assert(cb1 != nullptr);

  callback_t cb2;
  cb2 = std::move(cb1);
  assert(cb1 == nullptr);
  assert(cb2 != nullptr);
}

void swapped()
{
  callback_t cb1(function);
  assert(cb1 != nullptr);

  callback_t cb2;
  assert(cb2 == nullptr);

  swap(cb1, cb2);
  assert(cb1 == nullptr);
  assert(cb2 != nullptr);
}

void run_tests(int, char const* const*)
{
  empty_callback();
  function_callback();
  function_ptr_callback();
  functor_callback();
  lambda_callback();
  mutable_lambda_callback();

  move_construct();
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
