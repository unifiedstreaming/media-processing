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

#include "reader_utils.hpp"

#include "charclass.hpp"

namespace cuti
{

namespace detail
{

token_finder_t::token_finder_t(result_t<int>& result, bound_inbuf_t& buf)
: result_(result)
, buf_(buf)
{ }

void token_finder_t::start()
{
  int c{};
  while(buf_.readable() && is_whitespace(c = buf_.peek()))
  {
    buf_.skip();
  }

  if(!buf_.readable())
  {
    buf_.call_when_readable([this] { this->start(); });
    return;
  }

  // TODO: check for inline exception in buf_ and fail if so 

  result_.submit(c);
}
  
} // detail

} // cuti
