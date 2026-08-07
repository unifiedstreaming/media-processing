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

#include <cuti/function.hpp>

#include <functional>
#include <iostream>
#include <string>
#include <type_traits>

// enable assert()
#undef NDEBUG
#include <cassert>

namespace { // anonymous

using namespace cuti;

struct multiplier_t
{
  explicit multiplier_t(int by)
  : by_(by)
  { }

  int multiply(int i) const
  { return i * by_; }

public :
  int by_;
};

struct counter_t
{
  counter_t()
  : n_copies_(0)
  , n_moves_(0)
  { }

  counter_t(counter_t const& rhs)
  : n_copies_(rhs.n_copies_ + 1)
  , n_moves_(rhs.n_moves_)
  { }

  counter_t(counter_t&& rhs) noexcept
  : n_copies_(rhs.n_copies_)
  , n_moves_(rhs.n_moves_ + 1)
  { }

  counter_t& operator=(counter_t const& rhs)
  {
    n_copies_ = rhs.n_copies_ + 1;
    n_moves_ = rhs.n_moves_;
    return *this;
  }

  counter_t& operator=(counter_t&& rhs) noexcept
  {
    n_copies_ = rhs.n_copies_;
    n_moves_ = rhs.n_moves_ + 1;
    return *this;
  }

  int n_copies() const
  { return n_copies_; }

  int n_moves() const
  { return n_moves_; }

private :
  int n_copies_;
  int n_moves_;
};

int n_calls = 0;

void count_call()
{
  ++n_calls;
}

int pre_increment(int& i)
{
  ++i;
  return i;
}

int post_increment(int& i)
{
  auto result = i;
  ++i;
  return result;
}

std::string text;

void set_text(std::string arg)
{
  text = std::move(arg);
}

void test_emptiness()
{
  {
    function_t<void()> f;

    assert(bool(f) == false);
    assert(f == nullptr);
    assert(nullptr == f);
    assert(!(f != nullptr));
    assert(!(nullptr != f));
  }

  {
    function_t<void()> f = nullptr;
  
    assert(bool(f) == false);
    assert(f == nullptr);
    assert(nullptr == f);
    assert(!(f != nullptr));
    assert(!(nullptr != f));
  }

  {
    function_t<void()> f = count_call;
  
    assert(bool(f) == true);
    assert(!(f == nullptr));
    assert(!(nullptr == f));
    assert(f != nullptr);
    assert(nullptr != f);
  }

  {
    function_t<void()> f = [] { count_call(); };
  
    assert(bool(f) == true);
    assert(!(f == nullptr));
    assert(!(nullptr == f));
    assert(f != nullptr);
    assert(nullptr != f);
  }
}

void test_simple_calls()
{
  {
    function_t<void()> f = count_call;

    n_calls = 0;
    f();
    assert(n_calls == 1);
  }

  {
    function_t<void()> f = [] { count_call(); };

    n_calls = 0;
    f();
    assert(n_calls == 1);
  }
}

void test_copying()
{
  {
    function_t<void()> f1 = count_call;
    function_t<void()> f2 = f1;
  
    n_calls = 0;
    f1();
    f2();
    assert(n_calls == 2);
  }

  {
    function_t<void()> f1 = [] { count_call(); };
    function_t<void()> f2 = f1;
  
    n_calls = 0;
    f1();
    f2();
    assert(n_calls == 2);
  }

  {
    function_t<void()> f1 = count_call;
    function_t<void()> f2;
    assert(f1 != nullptr);
    assert(f2 == nullptr);
  
    f2 = f1;
    assert(f1 != nullptr);
    assert(f2 != nullptr);
    
    n_calls = 0;
    f1();
    f2();
    assert(n_calls == 2);
  }

  {
    function_t<void()> f1 = [] { count_call(); };
    function_t<void()> f2;
    assert(f1 != nullptr);
    assert(f2 == nullptr);
  
    f2 = f1;
    assert(f1 != nullptr);
    assert(f2 != nullptr);
    
    n_calls = 0;
    f1();
    f2();
    assert(n_calls == 2);
  }
}
  
void test_moving()
{
  static_assert(std::is_nothrow_move_constructible_v<function_t<void()>>);

  {
    function_t<void()> f1 = count_call;
    function_t<void()> f2 = std::move(f1);
    assert(f1 == nullptr);
    assert(f2 != nullptr);
  
    n_calls = 0;
    f2();
    assert(n_calls == 1);
  }

  {
    function_t<void()> f1 = [] { count_call(); };
    function_t<void()> f2 = std::move(f1);
    assert(f1 == nullptr);
    assert(f2 != nullptr);
  
    n_calls = 0;
    f2();
    assert(n_calls == 1);
  }

  static_assert(std::is_nothrow_move_assignable_v<function_t<void()>>);

  {
    function_t<void()> f1 = count_call;
    function_t<void()> f2;
    assert(f1 != nullptr);
    assert(f2 == nullptr);
  
    f2 = std::move(f1);
    assert(f1 == nullptr);
    assert(f2 != nullptr);
    
    n_calls = 0;
    f2();
    assert(n_calls == 1);
  }

  {
    function_t<void()> f1 = [] { count_call(); };
    function_t<void()> f2;
    assert(f1 != nullptr);
    assert(f2 == nullptr);
  
    f2 = std::move(f1);
    assert(f1 == nullptr);
    assert(f2 != nullptr);
    
    n_calls = 0;
    f2();
    assert(n_calls == 1);
  }
}

void test_pass_by_reference()
{
  {
    function_t<void(int&)> f = pre_increment;
    int i = 42;
    f(i);
    assert(i == 43);
  }

  {
    function_t<void(int&)> f = [](int& i) { pre_increment(i); };
    int i = 42;
    f(i);
    assert(i == 43);
  }
}

void test_return_value()
{
  {
    function_t<int(int&)> f = pre_increment;
    int i = 42;

    int r = f(i);

    assert(r == 43);
    assert(i == 43);
  }

  {
    function_t<int(int&)> f = [](int& i) { return pre_increment(i); };
    int i = 42;

    int r = f(i);

    assert(r == 43);
    assert(i == 43);
  }

  {
    function_t<int(int&)> f = post_increment;
    int i = 42;

    int r = f(i);

    assert(r == 42);
    assert(i == 43);
  }

  {
    function_t<int(int&)> f = [](int& i) { return post_increment(i); };
    int i = 42;

    int r = f(i);

    assert(r == 42);
    assert(i == 43);
  }
}

void test_pass_by_value()
{
  {
    function_t<void(std::string)> f = set_text;

    text = "";
    f("Wolfgang Amadeus Mozart");
    assert(text == "Wolfgang Amadeus Mozart");
  }

  {
    function_t<void(std::string)> f =
      [](std::string str) { set_text(std::move(str)); }; 

    text = "";
    f("Wolfgang Amadeus Mozart");
    assert(text == "Wolfgang Amadeus Mozart");
  }
}
  
void test_pass_by_const_reference()
{
  {
    function_t<void(std::string const&)> f = set_text;

    text = "";
    f("Wolfgang Amadeus Mozart");
    assert(text == "Wolfgang Amadeus Mozart");
  }

  {
    function_t<void(std::string const&)> f =
      [](std::string const& str) { set_text(str); }; 

    text = "";
    f("Wolfgang Amadeus Mozart");
    assert(text == "Wolfgang Amadeus Mozart");
  }
}
  
void test_pass_by_rvalue_reference()
{
  {
    function_t<void(std::string&&)> f = set_text;

    text = "";
    f("Wolfgang Amadeus Mozart");
    assert(text == "Wolfgang Amadeus Mozart");
  }

  {
    function_t<void(std::string&&)> f =
      [](std::string&& str) { set_text(std::move(str)); }; 

    text = "";
    f("Wolfgang Amadeus Mozart");
    assert(text == "Wolfgang Amadeus Mozart");
  }
}

void test_wrapped_noncopyable()
{
  int i = 17;

  auto lambda = [&i, p = std::make_unique<int>(42) ] { i = *p; };
  static_assert(!std::is_copy_constructible_v<decltype(lambda)>);

  function_t<void()> f = std::move(lambda);
  f();

  assert(i == 42);
}

void test_wrapped_null_function()
{
  {
    function_t<void(int, int)> f1 = nullptr;
    function_t<void(short, short)> f2 = f1;
    assert(f2 == nullptr);
  }

  {
    std::function<void(int, int)> f1 = nullptr;
    function_t<void(short, short)> f2 = f1;
    assert(f2 == nullptr);
  }
}

void test_pointer_to_member_function()
{
  {
    function_t<int(multiplier_t const&, int)> f = &multiplier_t::multiply;
    multiplier_t multiplier{6};
    int r = f(multiplier, 7);
    assert(r == 42);
  }

  {
    function_t<int(multiplier_t const*, int)> f = &multiplier_t::multiply;
    multiplier_t multiplier{6};
    int r = f(&multiplier, 7);
    assert(r == 42);
  }
}

void test_pointer_to_data_member()
{
  {
    function_t<int(multiplier_t const&)> f = &multiplier_t::by_;
    multiplier_t multiplier{6};

    assert(multiplier.by_ == 6);
    int r = f(multiplier);
    assert(r == 6);
  }

  {
    function_t<int(multiplier_t const*)> f = &multiplier_t::by_;
    multiplier_t multiplier{6};

    assert(multiplier.by_ == 6);
    int r = f(&multiplier);
    assert(r == 6);
  }

  {
    function_t<int&(multiplier_t&)> f = &multiplier_t::by_;
    multiplier_t multiplier{6};

    assert(multiplier.by_ == 6);
    f(multiplier) = 5;
    assert(multiplier.by_ == 5);
  }

  {
    function_t<int&(multiplier_t*)> f = &multiplier_t::by_;
    multiplier_t multiplier{6};

    assert(multiplier.by_ == 6);
    f(&multiplier) = 5;
    assert(multiplier.by_ == 5);
  }
}

void test_copies_and_moves()
{
  {
    // passing an lvalue by value
    function_t<void(counter_t)> f = [](counter_t const& delivered) {
      assert(delivered.n_copies() == 1);
      assert(delivered.n_moves() == 1);
    };

    counter_t arg{};
    f(arg);
  }

  {
    // passing an rvalue by value
    function_t<void(counter_t)> f = [](counter_t const& delivered) {
      assert(delivered.n_copies() == 0);
      assert(delivered.n_moves() == 1);
    };

    f(counter_t{});
  }

  {
    // passing by const reference
    function_t<void(counter_t const&)> f = [](counter_t const& delivered) {
      assert(delivered.n_copies() == 0);
      assert(delivered.n_moves() == 0);
    };

    counter_t arg{};
    f(arg);
  }

  {
    // passing by rvalue reference
    function_t<void(counter_t&&)> f = [](counter_t const& delivered) {
      assert(delivered.n_copies() == 0);
      assert(delivered.n_moves() == 0);
    };

    f(counter_t{});
  }
}
  
int run_tests(int /* argc */, char const* const* /* argv */)
{
  test_emptiness();
  test_simple_calls();
  test_copying();
  test_moving();

  test_pass_by_reference();
  test_return_value();

  test_pass_by_value();
  test_pass_by_const_reference();
  test_pass_by_rvalue_reference();

  test_wrapped_noncopyable();
  test_wrapped_null_function();

  test_pointer_to_member_function();
  test_pointer_to_data_member();

  test_copies_and_moves();

  return 0;
}

} // anonymous

int main(int argc, char* argv[])
{
  try
  {
    return run_tests(argc, argv);
  }
  catch(std::exception const& ex)
  {
    std::cerr << argv[0] << ": exception: " << ex.what() << std::endl;
  }

  return 1;
}
