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

#include <cuti/viewbuf.hpp>

#include <iostream>
#include <cstring>

// Enable assert()
#undef NDEBUG
#include <cassert>

namespace // anonymous
{

using namespace cuti;

void empty_viewbuf()
{
  viewbuf_t vb(nullptr, nullptr);
  assert(vb.sgetc() == std::streambuf::traits_type::eof());
}

void nonempty_viewbuf()
{
  char buf[] = "\xFFHello world";
  viewbuf_t vb(buf, buf + std::strlen(buf));
  for(const char *p = buf; *p != '\0'; ++p)
  {
    assert(vb.sbumpc() == static_cast<unsigned char>(*p));
  }
  assert(vb.sgetc() == std::streambuf::traits_type::eof());
}

void viewbuf_from_cstr()
{
  char buf[] = "\xFFHello world";
  viewbuf_t vb(buf);
  for(const char *p = buf; *p != '\0'; ++p)
  {
    assert(vb.sbumpc() == static_cast<unsigned char>(*p));
  }
  assert(vb.sgetc() == std::streambuf::traits_type::eof());
}

void run_tests(int /* argc */, char const* const* /* argv */)
{
  empty_viewbuf();
  nonempty_viewbuf();
  viewbuf_from_cstr();
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

