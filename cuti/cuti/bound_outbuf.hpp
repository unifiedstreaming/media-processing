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

#ifndef CUTI_BOUND_OUTBUF_HPP_
#define CUTI_BOUND_OUTBUF_HPP_

#include "callback.hpp"
#include "linkage.h"
#include "nb_outbuf.hpp"

#include <utility>

namespace cuti
{

struct scheduler_t;

/*
 * A vehicle for referencing an nb_outbuf_t and its currently associated
 * scheduler.
 */
struct CUTI_ABI bound_outbuf_t
{
  bound_outbuf_t(nb_outbuf_t& outbuf, scheduler_t& scheduler)
  : outbuf_(outbuf)
  , scheduler_(scheduler)
  { }

  bound_outbuf_t(bound_outbuf_t const&) = delete;
  bound_outbuf_t& operator=(bound_outbuf_t const&) = delete;
  
  bool writable() const
  {
    return outbuf_.writable();
  }

  void put(char c) const
  {
    outbuf_.put(c);
  }

  char const* write(char const* first, char const* last)
  {
    return outbuf_.write(first, last);
  }

  void start_flush()
  {
    outbuf_.start_flush();
  }

  void call_when_writable(callback_t callback)
  {
    outbuf_.call_when_writable(scheduler_, std::move(callback));
  }

  void cancel_when_writable() noexcept
  {
    outbuf_.cancel_when_writable();
  }

private :
  nb_outbuf_t& outbuf_;
  scheduler_t& scheduler_;
};

} // cuti

#endif