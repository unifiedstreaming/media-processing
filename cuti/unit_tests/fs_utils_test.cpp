/*
 * Copyright (C) 2021 CodeShop B.V.
 *
 * This file is part of the cuti library.
 *
 * The cuti library is free software: you can redistribute it
 * and/or modify it under the terms of version 2 of the GNU General
 * Public License as published by the Free Software Foundation.
 *
 * The cuti library is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See version 2 of the GNU General Public License for more details.
 *
 * You should have received a copy of version 2 of the GNU General
 * Public License along with the cuti library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "fs_utils.hpp"

#include <iostream>

// enable assert()
#undef NDEBUG
#include <cassert>

#define PRINT 1
#undef PRINT

namespace // anonymous
{

void test_current_directory()
{
  std::string dir = cuti::current_directory();
#if PRINT
  std::cout << "current directory: " << dir << std::endl;
#endif

  std::string abs = cuti::absolute_path(dir.c_str());
  assert(dir == abs);
}  
  
void test_absolute_path(char const* path)
{
  std::string abs1 = cuti::absolute_path(path);
#if PRINT
  std::cout << path << " -> " << abs1 << std::endl;
#endif

  std::string abs2 = cuti::absolute_path(abs1.c_str());
  assert(abs1 == abs2);
}

} // anonymous

int main()
{
  test_current_directory();

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

  return 0;
}
