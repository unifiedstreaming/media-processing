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

#include <cuti/stringprintf.hpp>

#include <iostream>

// Enable assert()
#undef NDEBUG
#include <cassert>

namespace // anonymous
{

using namespace cuti;

void simple_printf()
{
  auto result = stringprintf("hello %s %d", "world", 42);
  assert(result == "hello world 42");
}

void failing_printf()
{
  wchar_t invalid[] = { 0xFFFF, 42, 43, 44, 0 };
  auto result = stringprintf("%ls", invalid);
  assert(result == "vsnprintf() encoding error");
}

void big_printf()
{
  std::string big(10000, 'i');
  auto result = stringprintf("hello b%sg world", big.c_str());
  assert(result == "hello b" + big + "g world");
}

void run_tests(int /* argc */, char const* const* /* argv */)
{
  simple_printf();
  failing_printf();
  big_printf();
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

// End Of File

