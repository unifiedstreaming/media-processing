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

#include <cuti/string_builder.hpp>

#include <exception>
#include <iostream>
#include <string_view>

// enable assert()
#undef NDEBUG
#include <cassert>

namespace // anonymous
{

using namespace cuti;

void test_once()
{
  string_builder_t builder;
  builder << "error " << 42;

  assert(std::string_view(builder.begin(), builder.end()) == "error 42");
  assert(builder.result() == "error 42");
}

void test_twice()
{
  string_builder_t builder;
  builder << "error " << 42;

  assert(std::string_view(builder.begin(), builder.end()) == "error 42");
  assert(builder.result() == "error 42");

  assert(std::string_view(builder.begin(), builder.end()) == "error 42");
  assert(builder.result() == "error 42");
}

void test_again()
{
  string_builder_t builder;
  builder << "error " << 42;
  assert(std::string_view(builder.begin(), builder.end()) == "error 42");
  assert(builder.result() == "error 42");

  builder << " (as expected!)";
  assert(std::string_view(builder.begin(), builder.end()) ==
    "error 42 (as expected!)");
  assert(builder.result() ==
    "error 42 (as expected!)");
}

void run_tests(int, char const* const*)
{
  test_once();
  test_twice();
  test_again();
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
