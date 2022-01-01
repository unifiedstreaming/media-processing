/*
 * Copyright (C) 2021-2022 CodeShop B.V.
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

#ifndef CUTI_MEMBUF_HPP_
#define CUTI_MEMBUF_HPP_

#include "linkage.h"

#include <streambuf>

namespace cuti
{

/*
 * An output stream buffer that generates a character array.
 */
struct CUTI_ABI membuf_t : std::streambuf
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

} // namespace cuti

#endif
