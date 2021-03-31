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

#ifndef XES_MEMBUF_HPP_
#define XES_MEMBUF_HPP_

#include <streambuf>

namespace xes
{

struct membuf_t : std::streambuf
{
  membuf_t();

  membuf_t(membuf_t const&) = delete;
  membuf_t& operator=(membuf_t const&) = delete;
  
  char const* begin() const
  {
    return buf_;
  }

  char const* end() const
  {
    return this->pptr();
  }

  ~membuf_t() override;

protected :
  int_type overflow(int_type c) override;

private :
  char inline_buf_[256];
  char* buf_;
};
    
} // namespace xes

#endif
