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

#ifndef CUTI_NB_SINK_HPP_
#define CUTI_NB_SINK_HPP_

#include "callback.hpp"
#include "cancellation_ticket.hpp"
#include "linkage.h"

#include <iosfwd>

namespace cuti
{

struct scheduler_t;

/*
 * Non-blocking character sink interface
 */
struct CUTI_ABI nb_sink_t
{
  nb_sink_t()
  { }

  nb_sink_t(nb_sink_t const&) = delete;
  nb_sink_t& operator=(nb_sink_t const&) = delete;

  /*
   * Tries to write some of the bytes in range [first, last>.  next is
   * set to either the end of the range that was written or to nullptr
   * if the call would block.
   * Returns 0 on success; on error, next is set to last and a system
   * error code is returned.  Please note that refusing
   * to block is not an error.
   */
  virtual int write(
    char const* first, char const* last, char const*& next) = 0;
  
  /*
   * Requests a one-time callback for when the sink is detected to
   * be writable, returning a cancellation ticket that may be used to
   * cancel the callback by calling scheduler.cancel().
   */
  virtual cancellation_ticket_t call_when_writable(
    scheduler_t& scheduler, callback_t callback) = 0;
   
  virtual void print(std::ostream& os) const = 0;

  virtual ~nb_sink_t();
};

} // cuti

#endif
