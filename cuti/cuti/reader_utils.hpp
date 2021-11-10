/*
 * Copyright (C) 2021 CodeShop B.V.
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

#ifndef CUTI_READER_UTILS_HPP_
#define CUTI_READER_UTILS_HPP_

#include "bound_inbuf.hpp"
#include "linkage.h"
#include "result.hpp"

namespace cuti
{

namespace detail
{

struct CUTI_ABI token_finder_t
{
  using value_t = int;

  token_finder_t(result_t<int>& result, bound_inbuf_t& buf);

  token_finder_t(token_finder_t const&) = delete;
  token_finder_t& operator=(token_finder_t const&) = delete;

  /*
   * Skips whitespace and eventually submits the first non-whitespace
   * character from buf (which could be eof).  At that position in
   * buf, buf.readable() will be true and buf.peek() will equal the
   * submitted value.
   */
  void start();

private :
  result_t<int>& result_;
  bound_inbuf_t& buf_;
};

} // detail

} // cuti

#endif
