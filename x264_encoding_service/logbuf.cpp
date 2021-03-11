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

namespace xes
{

logbuf_t::logbuf_t()
: std::streambuf()
{
  char *end_inline_buf = inline_buf_ + sizeof inline_buf_;

  // Fill with zeros, making sure the final cell is not overwritten,
  // to cater for c_str().
  std::fill(inline_buf_, end_inline_buf, '\0');

  this->setp(inline_buf_, inline_buf_, end_inline_buf - 1);
}

logbuf_t::~logbuf_t()
{
  char* buf = this->pbase();
  if(buf != inline_buf_)
  {
    delete[] buf;
  }
}

int logbuf_t::overflow(int c)
{
  char* buf = this->pbase();
  char* pptr = this->pptr();
  char* epptr = this->epptr();

  if(pptr == epptr)
  {
    std::size_t length = pptr - buf;
    std::size_t old_size = length + 1;
    std::size_t new_size = old_size + old_size / 2 + sizeof inline_buf_;
    char* new_buf = new char[new_size];
    std::copy(buf, pptr, new_buf);

    // Fill remainder with zeros, making sure the final cell is not
    // overwritten, to cater for c_str().
    std::fill(new_buf + length, new_buf + new_size, '\0');

    if(buf != inline_buf_)
    {
      delete[] buf;
    }

    buf = new_buf;
    pptr = new_buf + length;
    epptr = new_buf + new_size - 1;
  }

  if(c != traits_type::eof())
  {
    *pptr = traits_type::to_char_type(c);
    ++pptr;
  }

  this->setp(buf, pptr, epptr);

  return traits_type::not_eof(c);
}

} // namespace xes
