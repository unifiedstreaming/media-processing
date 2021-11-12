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

#include "writer_utils.hpp"

namespace cuti
{

namespace detail
{

literal_writer_t::literal_writer_t(result_t<void>& result, bound_outbuf_t& buf)
: result_(result)
, buf_(buf)
{ }

void literal_writer_t::write_chars()
{
  while(first_ != last_ && buf_.writable())
  {
    buf_.put(*first_);
    ++first_;
  }

  if(first_ != last_)
  {
    buf_.call_when_writable([this] { this->write_chars(); });
    return;
  }

  result_.submit();
}

} // detail

} // cuti
