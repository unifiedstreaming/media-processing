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

#include "streambuf_backend.hpp"
#include "format.hpp"

#include <ostream>

namespace xes
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

  auto now = std::chrono::system_clock::now();

  format_timepoint(*sb_, now);
  sb_->sputc(' ');
  format_loglevel(*sb_, level);
  sb_->sputc(' ');
  sb_->sputn(begin_msg, end_msg - begin_msg);
  sb_->sputc('\n');

  sb_->pubsync();
}

} // namespace xes
