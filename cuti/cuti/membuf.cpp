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

#include "membuf.hpp"

#include <algorithm>

namespace cuti
{

membuf_t::membuf_t()
: std::streambuf()
, buf_(inline_buf_)
{
  this->setg(buf_, buf_, buf_);
  this->setp(buf_, buf_ + sizeof inline_buf_);
}

void membuf_t::clear()
{
  char* epptr = this->epptr();
  
  this->setg(buf_, buf_, buf_);
  this->setp(buf_ , epptr);
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
  char* gptr = this->gptr();
  char* pptr = this->pptr();
  char* epptr = this->epptr();

  if(pptr == epptr)
  {
    std::size_t count = pptr - gptr;
    std::size_t capacity = epptr - buf_;
    std::size_t minimum_capacity = count + count / 2 + 15;

    if(capacity >= minimum_capacity)
    {
      // shift left
      std::copy(gptr, pptr, buf_);
    }
    else
    {
      // reallocate
      char* new_buf = new char[minimum_capacity];
      std::copy(gptr, pptr, new_buf);

      if(buf_ != inline_buf_)
      {
        delete[] buf_;
      }

      buf_ = new_buf;
      epptr = buf_ + minimum_capacity;
    }

    gptr = buf_;
    pptr = buf_ + count;
  }

  if(c != traits_type::eof())
  {
    *pptr++ = traits_type::to_char_type(c);
  }

  this->setg(buf_, gptr, pptr);
  this->setp(pptr, epptr);

  return traits_type::not_eof(c);
}

membuf_t::int_type membuf_t::underflow()
{
  char* gptr = this->gptr();
  char* pptr = this->pptr();
  this->setg(buf_, gptr, pptr);

  return gptr != pptr ? traits_type::to_int_type(*gptr) : traits_type::eof();
}

} // namespace cuti
