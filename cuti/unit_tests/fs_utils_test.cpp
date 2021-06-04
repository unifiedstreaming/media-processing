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

#include "fs_utils.hpp"

#include <exception>
#include <iostream>

// enable assert()
#undef NDEBUG
#include <cassert>

#define PRINT 1
#undef PRINT

namespace // anonymous
{

using namespace cuti;

void test_current_directory()
{
  std::string dir = current_directory();
#if PRINT
  std::cout << "current directory: " << dir << std::endl;
#endif

  auto abs = absolute_path_t(dir);
  assert(!abs.empty());
  assert(dir == abs.value());
}

void test_change_directory()
{
  std::string dir = current_directory();

  change_directory(".");
  assert(current_directory() == dir);

  change_directory(dir.c_str());
  assert(current_directory() == dir);
}

void test_absolute_path(char const* path)
{
  auto abs1 = absolute_path_t(path);
  assert(!abs1.empty());
#if PRINT
  std::cout << path << " -> " << abs1.value() << std::endl;
#endif

  auto abs2 = absolute_path_t(abs1.value());
  assert(!abs2.empty());
  assert(abs1.value() == abs2.value());
}

void run_tests(int, char const* const*)
{
  test_current_directory();

  test_change_directory();

  test_absolute_path("simple");
  test_absolute_path("in/subdir");
  test_absolute_path("trailing/slash/");

  test_absolute_path("./leading/dot");
  test_absolute_path("middle/./dot");
  test_absolute_path("trailing/dot/.");

  test_absolute_path("../leading/dotdot");
  test_absolute_path("middle/../dotdot");
  test_absolute_path("trailing/dotdot/..");

  test_absolute_path("/");
  test_absolute_path("/.");
  test_absolute_path("/..");

  test_absolute_path("/inroot");
  test_absolute_path("/./dotroot");
  test_absolute_path("/rootdot/.");
  test_absolute_path("/../dotdotroot");
  test_absolute_path("/rootdotdot/..");

#ifdef _WIN32
  test_absolute_path("back\\slash");
  test_absolute_path("trailing\\backslash\\");
  test_absolute_path("C:\\");
  test_absolute_path("C:");
  test_absolute_path("c:cfile");
  test_absolute_path("A:");
#endif
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
