/*
 * Copyright (C) 2021 CodeShop B.V.
 *
 * This file is part of the x264_encoding_service.
 *
 * The x264_encoding_service is free software: you can redistribute it
 * and/or modify it under the terms of version 2 of the GNU General
 * Public License as published by the Free Software Foundation.
 *
 * The x264_encoding_service is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See version 2 of the GNU General Public License for more details.
 *
 * You should have received a copy of version 2 of the GNU General
 * Public License along with the x264_encoding_service.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "logbuf.hpp"

#include <algorithm>
#include <string>

// enable assert()
#undef NDEBUG
#include <cassert>

namespace // anonymous
{

void test_short()
{
  xes::logbuf_t buf;
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
  xes::logbuf_t buf;
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
  xes::logbuf_t buf;
  std::string str;

  for(unsigned int i = 0; i != 65536; ++i)
  {
    char c = static_cast<char>(i % 127) + ' ';
    buf.sputc(c);
    str += c;
  }

  assert(std::equal(buf.begin(), buf.end(), str.begin(), str.end()));
}

} // anonymous

int main()
{
  test_short();
  test_zeros();
  test_long();

  return 0;
}
