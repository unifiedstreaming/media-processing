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

#include "membuf.hpp"

#include "charclass.hpp"

#include <algorithm>

namespace cuti
{

membuf_t::membuf_t()
: std::streambuf()
, buf_(inline_buf_)
{
  this->setp(inline_buf_, inline_buf_ + sizeof inline_buf_);
}

membuf_t::~membuf_t()
{
  if(buf_ != inline_buf_)
  {
    delete[] buf_;
  }
}

membuf_t::int_type membuf_t::overflow(int_type c)
{
  char* pptr = this->pptr();
  char* epptr = this->epptr();

  if(pptr == epptr)
  {
    std::size_t old_size = pptr - buf_;
    std::size_t new_size = old_size + old_size / 2 + sizeof inline_buf_;
    char* new_buf = new char[new_size];
    std::copy(buf_, pptr, new_buf);

    if(buf_ != inline_buf_)
    {
      delete[] buf_;
    }

    buf_ = new_buf;
    pptr = new_buf + old_size;
    epptr = new_buf + new_size;
  }

  if(c != eof)
  {
    *pptr = traits_type::to_char_type(c);
    ++pptr;
  }

  this->setp(pptr, epptr);

  return traits_type::not_eof(c);
}

} // namespace cuti
