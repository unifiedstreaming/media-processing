/*
 * Copyright (C) 2021 CodeShop B.V.
 *
 * This file is part of the cuti library.
 *
 * The cuti library is free software: you can redistribute it
 * and/or modify it under the terms of version 2 of the GNU General
 * Public License as published by the Free Software Foundation.
 *
 * The cuti library is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See version 2 of the GNU General Public License for more details.
 *
 * You should have received a copy of version 2 of the GNU General
 * Public License along with the cuti library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "default_backend.hpp"

#include <iostream>

namespace cuti
{

default_backend_t::default_backend_t(char const* argv0)
: argv0_(argv0)
, sb_(std::cerr.rdbuf())
{ }

void default_backend_t::report(loglevel_t /* ignored */,
                               char const* begin_msg, char const* end_msg)
{
  if(sb_ != nullptr)
  {
    sb_->sputn(argv0_.data(), argv0_.size());
    sb_->sputc(':');
    sb_->sputc(' ');
    sb_->sputn(begin_msg, end_msg - begin_msg);
    sb_->sputc('\n');
    sb_->pubsync();
  }
}

} // cuti
