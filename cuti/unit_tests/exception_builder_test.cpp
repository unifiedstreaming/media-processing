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

#include <cuti/exception_builder.hpp>

#include <iostream>
#include <stdexcept>
#include <string>

#undef NDEBUG
#include <cassert>

namespace { // anonymous

using namespace cuti;

struct ex0_t : std::runtime_error
{
  explicit ex0_t(std::string const& msg)
  : std::runtime_error(msg)
  { }
};

struct ex1_t : std::runtime_error
{
  ex1_t(int arg1, std::string const& msg)
  : std::runtime_error(msg)
  , arg1_(arg1)
  { }

  int arg1() const
  { return arg1_; }

private :
  int arg1_;
};

struct ex1_factory_t
{
  explicit ex1_factory_t(int arg1)
  : arg1_(arg1)
  { }

  ex1_t operator()(std::string const& msg) const
  { return ex1_t{arg1_, msg}; }

private :
  int arg1_;
};
    
struct ex2_t : std::runtime_error
{
  ex2_t(int arg1, std::string const& msg, int arg3)
  : std::runtime_error(msg)
  , arg1_(arg1)
  , arg3_(arg3)
  { }

  int arg1() const
  { return arg1_; }

  int arg3() const
  { return arg3_; }

private :
  int arg1_;
  int arg3_;
};

struct ex2_factory_t
{
  ex2_factory_t(int arg1, int arg3)
  : arg1_(arg1)
  , arg3_(arg3)
  { }

  ex2_t operator()(std::string const& msg) const
  { return ex2_t{arg1_, msg, arg3_}; }

private :
  int arg1_;
  int arg3_;
};
    
void test_ex0()
{
  std::string msg = "Hello from ex0";
  bool caught = false;

  try
  {
    exception_builder_t<ex0_t> builder;
    builder << msg;
    builder.explode();
  }
  catch(ex0_t const& ex)
  {
    assert(ex.what() == msg);
    caught = true;
  }

  assert(caught);
}

void test_ex1()
{
  int arg1 = 42;
  std::string msg = "Hello from ex1";
  bool caught = false;

  try
  {
    exception_builder_t<ex1_t, ex1_factory_t> builder{arg1};
    builder << msg;
    builder.explode();
  }
  catch(ex1_t const& ex)
  {
    assert(ex.arg1() == arg1);
    assert(ex.what() == msg);
    caught = true;
  }

  assert(caught);
}

void test_ex2()
{
  int arg1 = 42;
  std::string msg = "Hello from ex2";
  int arg3 = 4711;
  bool caught = false;

  try
  {
    exception_builder_t<ex2_t, ex2_factory_t> builder{arg1, arg3};
    builder << msg;
    builder.explode();
  }
  catch(ex2_t const& ex)
  {
    assert(ex.arg1() == arg1);
    assert(ex.what() == msg);
    assert(ex.arg3() == arg3);
    caught = true;
  }

  assert(caught);
}

int run_tests()
{
  test_ex0();
  test_ex1();
  test_ex2();

  return 0;
}

} // anonymous

int main(int, char* argv[])
{
  try
  {
    return run_tests();
  }
  catch(std::exception const& ex)
  {
    std::cerr << argv[0] << ": exception: " << ex.what() << std::endl;
  }

  return 1;
}
