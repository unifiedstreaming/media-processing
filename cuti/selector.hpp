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

#ifndef CUTI_SELECTOR_HPP_
#define CUTI_SELECTOR_HPP_

#include "io_scheduler.hpp"
#include "linkage.h"
#include "loglevel.hpp"
#include "socket_nifty.hpp"

#include <chrono>

namespace cuti
{

struct logging_context_t;

/*
 * Abstract selector interface
 */
struct CUTI_ABI selector_t : io_scheduler_t 
{
  using timeout_t = std::chrono::system_clock::duration;

  selector_t()
  { }

  selector_t(selector_t const&) = delete;
  selector_t& operator=(selector_t const&) = delete;

  /*
   * Returns true if there are any pending callbacks, false otherwise.
   */
  virtual bool has_work() const noexcept = 0;

  /*
   * Waits for no longer than <timeout> for an I/O event to occur,
   * returning either the non-empty callback for the first detected
   * event, or an empty callback if no event was detected yet.
   *
   * If <timeout> is negative, no timeout is applied; if <timeout> is
   * zero, this function does not block.
   *
   * Callers are expected to deal with spurious early returns: please
   * keep in mind that, in rare cases, this function may return an
   * empty callback before the timeout is reached.
   *
   * Precondition: this->has_work()
   */
  virtual callback_t select(timeout_t timeout) = 0;

  virtual ~selector_t();
};

/*
 * Keep invoking the callbacks returned from <selector>.select() for
 * no longer than <timeout>.  <timeout> must not be negative; if it is
 * zero, this function invokes at most one callback and does not wait
 * for further events.  Returns early when <selector> runs out of
 * work.
 * Usually, you'll want more than just a (single) selector to
 * drive an event loop; this function mainly exists for testing
 * purposes.
 */
CUTI_ABI
void run_selector(logging_context_t const& context,
                  loglevel_t loglevel,
                  selector_t& selector,
                  selector_t::timeout_t timeout);

} // cuti

#endif
