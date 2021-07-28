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

#ifndef CUTI_ASYNC_SOURCE_HPP_
#define CUTI_ASYNC_SOURCE_HPP_

#include "async_inbuf.hpp"
#include "linkage.h"

#include <utility>

namespace cuti
{

/*
 * A temporary association of an async_inbuf and a scheduler.
 */
struct CUTI_ABI async_source_t
{
  async_source_t(async_inbuf_t& inbuf, scheduler_t& scheduler)
  : inbuf_(inbuf)
  , scheduler_(scheduler)
  { }

  bool readable() const
  {
    return inbuf_.readable();
  }

  int peek() const
  {
    return inbuf_.peek();
  }

  void skip()
  {
    inbuf_.skip();
  }

  char* read(char* first, char const* last)
  {
    return inbuf_.read(first, last);
  }

  void call_when_readable(callback_t callback)
  {
    inbuf_.call_when_readable(scheduler_, std::move(callback));
  }

private :
  async_inbuf_t& inbuf_;
  scheduler_t& scheduler_;
};

} // cuti

#endif
