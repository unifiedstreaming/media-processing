/*
 * Copyright (C) 2021-2024 CodeShop B.V.
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

#include "streambuf_backend.hpp"
#include "format.hpp"
#include "membuf.hpp"

#include <ostream>

namespace cuti
{

streambuf_backend_t::streambuf_backend_t(std::streambuf* sb)
: sb_(sb)
{ }

streambuf_backend_t::streambuf_backend_t(std::ostream& os)
: streambuf_backend_t(os.rdbuf())
{ }

void streambuf_backend_t::report(loglevel_t level,
                                 char const* begin_msg, char const* end_msg)
{
  if(sb_ == nullptr)
  {
    return;
  }

  membuf_t membuf;

  auto now = cuti_clock_t::now();
  format_time_point(membuf, now);

  membuf.sputc(' ');
  format_loglevel(membuf, level);

  membuf.sputc(' ');
  membuf.sputn(begin_msg, end_msg - begin_msg);
  membuf.sputc('\n');

  sb_->sputn(membuf.begin(), membuf.end() - membuf.begin());
  sb_->pubsync();
}

} // namespace cuti
