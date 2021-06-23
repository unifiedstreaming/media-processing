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

#include <cuti/membuf.hpp>

#include <algorithm>
#include <exception>
#include <iostream>
#include <string>

// enable assert()
#undef NDEBUG
#include <cassert>

namespace // anonymous
{

void test_short()
{
  cuti::membuf_t buf;
  std::string str;

  for(unsigned int i = 0; i != 128; ++i)
  {
    char c = static_cast<char>(i % 127) + ' ';
    buf.sputc(c);
    str += c;
  }

  assert(std::equal(buf.begin(), buf.end(), str.begin(), str.end()));
}

void test_zeros()
{
  cuti::membuf_t buf;
  std::string str;

  for(unsigned int i = 0; i != 128; ++i)
  {
    char c = '\0';
    buf.sputc(c);
    str += c;
  }

  assert(std::equal(buf.begin(), buf.end(), str.begin(), str.end()));
}

void test_long()
{
  cuti::membuf_t buf;
  std::string str;

  for(unsigned int i = 0; i != 65536; ++i)
  {
    char c = static_cast<char>(i % 127) + ' ';
    buf.sputc(c);
    str += c;
  }

  assert(std::equal(buf.begin(), buf.end(), str.begin(), str.end()));
}

void run_tests(int, char const* const*)
{
  test_short();
  test_zeros();
  test_long();
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
