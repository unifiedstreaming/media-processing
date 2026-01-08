/*
 * Copyright (C) 2021-2026 CodeShop B.V.
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

#include "flusher.hpp"

namespace cuti
{

flusher_t::flusher_t(result_t<void>& result, bound_outbuf_t& buf)
: result_(result)
, buf_(buf)
{ }

void flusher_t::start(stack_marker_t& base_marker)
{
  buf_.start_flush();
  this->check_flushed(base_marker);
}

void flusher_t::check_flushed(stack_marker_t& base_marker)
{
  if(!buf_.writable())
  {
    buf_.call_when_writable(
      [this](stack_marker_t& marker) { this->check_flushed(marker); }
    );
    return;
  }

  result_.submit(base_marker);
}

} // cuti
