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

#include "eof_reader.hpp"

#include "eof.hpp"
#include "parse_error.hpp"

namespace cuti
{

eof_reader_t::eof_reader_t(result_t<no_value_t>& result, bound_inbuf_t& buf)
: result_(result)
, buf_(buf)
{ }

void eof_reader_t::start()
{
  if(!buf_.readable())
  {
    buf_.call_when_readable([this] { this->start(); });
    return;
  }

  if(buf_.peek() != eof)
  {
    result_.fail(parse_error_t("<eof> expected"));
    return;
  }

  result_.submit();
}

} // cuti

    