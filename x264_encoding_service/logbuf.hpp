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

#ifndef LOGBUF_HPP_
#define LOGBUF_HPP_

#include <streambuf>

namespace xes
{

struct logbuf_t : std::streambuf
{
  logbuf_t();

  logbuf_t(logbuf_t const&) = delete;
  logbuf_t& operator=(logbuf_t const&) = delete;
  
  char const* begin() const
  {
    return buf_;
  }

  char const* end() const
  {
    return this->pptr();
  }

  char const* c_str() const
  {
    return buf_;
  }

  ~logbuf_t() override;

protected :
  int overflow(int c) override;

private :
  char inline_buf_[256];
  char* buf_;
};
    
} // namespace xes

#endif
