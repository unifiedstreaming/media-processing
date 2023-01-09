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

#ifndef CUTI_VIEWBUF_HPP_
#define CUTI_VIEWBUF_HPP_

#include "linkage.h"

#include <streambuf>

namespace cuti
{

/*
 * An input stream buffer that reads from a character array.
 */
struct CUTI_ABI viewbuf_t : std::streambuf
{
  viewbuf_t(char const* begin, char const* end);

  viewbuf_t(viewbuf_t const&) = delete;
  viewbuf_t& operator=(viewbuf_t const&) = delete;

protected :
  int_type underflow() override;
};

} // cuti

#endif
