/*
 * Copyright (C) 2021-2023 CodeShop B.V.
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

#include "viewbuf.hpp"

#include "charclass.hpp"

namespace cuti
{

viewbuf_t::viewbuf_t(char const* begin, char const* end)
: std::streambuf()
{
  // This is a read-only streambuf with inherited pbackfail(),
  // so casting away const is OK.
  this->setg(const_cast<char*>(begin), const_cast<char*>(begin),
             const_cast<char*>(end));
}

viewbuf_t::int_type viewbuf_t::underflow()
{
  auto gptr = this->gptr();
  return gptr == this->egptr() ?
         eof :
         traits_type::to_int_type(*gptr);
}

} // cuti
