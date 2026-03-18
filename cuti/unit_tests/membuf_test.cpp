/*
 * Copyright (C) 2019-2026 CodeShop B.V.
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

void empty_buf()
{
  cuti::membuf_t mb;
  assert(mb.sgetc() == std::streambuf::traits_type::eof());
}

void echo()
{
  std::string const str = "Hello world\xFF";
  cuti::membuf_t mb;
  
  for(char c : str)
  {
    mb.sputc(c);
  }

  std::size_t mb_size = mb.end() - mb.begin();
  assert(mb_size == str.size());
  assert(std::equal(mb.begin(), mb.end(), str.begin()));

  for(char c : str)
  {
    assert(mb.sbumpc() == std::streambuf::traits_type::to_int_type(c));
  }
  assert(mb.sgetc() == std::streambuf::traits_type::eof());
}

void bulk()
{
  std::size_t out = 0;
  std::size_t in = 0;
  cuti::membuf_t mb;

  for(int i = 0; i != 100; ++i)
  {
    for(int j = 0; j != 1000; ++j)
    {
      auto c = static_cast<char>(out);
      mb.sputc(c);
      ++out;
    }

    std::size_t k = in;
    for(char const* p = mb.begin(); p != mb.end(); ++p)
    {
      assert(k != out);
      auto c = static_cast<char>(k);
      assert(*p == c);
      ++k;
    }
    assert(k == out);

    for(int j = 0; j != 750; ++j)
    {
      auto c = static_cast<char>(in);
      assert(mb.sbumpc() == std::streambuf::traits_type::to_int_type(c));
      ++in;
    }
  }

  std::size_t k = in;
  for(char const* p = mb.begin(); p != mb.end(); ++p)
  {
    assert(k != out);
    auto c = static_cast<char>(k);
    assert(*p == c);
    ++k;
  }
  assert(k == out);

  for (int c1 = mb.sgetc();
       c1 != std::streambuf::traits_type::eof();
       c1 = mb.snextc())
  {
    assert(in != out);
    auto c2 = static_cast<char>(in);
    assert(c1 == std::streambuf::traits_type::to_int_type(c2));
    ++in;
  }
  assert(in == out);
}

void clear()
{
  std::string const str = "Hello world";
  cuti::membuf_t mb;
  
  for(char c : str)
  {
    mb.sputc(c);
  }

  std::size_t count = mb.end() - mb.begin();
  assert(count == str.length());

  mb.clear();
  
  count = mb.end() - mb.begin();
  assert(count == 0);
}

void run_tests(int, char const* const*)
{
  test_short();
  test_zeros();
  test_long();

  empty_buf();
  echo();
  bulk();
  clear();
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
