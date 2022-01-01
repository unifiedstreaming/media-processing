/*
 * Copyright (C) 2021-2022 CodeShop B.V.
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

#include "callback.hpp"
#include "chrono_types.hpp"
#include "linkage.h"
#include "socket_nifty.hpp"

namespace cuti
{

/*
 * Abstract network event selector interface
 */
struct CUTI_ABI selector_t
{
  selector_t()
  { }

  selector_t(selector_t const&) = delete;
  selector_t& operator=(selector_t const&) = delete;

  /*
   * Schedules a one-time callback for when fd is ready for writing.
   * Returns a cancellation ticket that may be passed to
   * cancel_when_writable().  The ticket remains valid until the
   * callback is selected.
   */
  virtual int call_when_writable(int fd, callback_t callback) = 0;

  /*
   * Cancels a callback scheduled with call_when_writable.
   */
  virtual void cancel_when_writable(int ticket) noexcept = 0;

  /*
   * Schedules a one-time callback for when fd is ready for reading.
   * Returns a cancellation ticket that may be passed to
   * cancel_when_readable().  The ticket remains valid until the
   * callback is selected.
   */
  virtual int call_when_readable(int fd, callback_t callback) = 0;

  /*
   * Cancels a callback scheduled with call_when_readable.
   */
  virtual void cancel_when_readable(int ticket) noexcept = 0;

  /*
   * Returns true if there are any pending callbacks, false otherwise.
   */
  virtual bool has_work() const noexcept = 0;

  /*
   * Waits for no longer than <timeout> for an I/O event to occur,
   * returning either the non-empty callback for the first detected
   * event, or an empty callback if no event was detected yet.
   *   
   * Spurious early returns are possible, so please keep in mind that,
   * in rare cases, this function may return an empty callback before
   * the timeout is reached.
   *
   * If <timeout> is negative, no timeout is applied; if <timeout> is
   * zero, this function does not block.
   *
   * Precondition: this->has_work()
   */
  virtual callback_t select(duration_t timeout) = 0;

  virtual ~selector_t();

protected :
  enum class event_t { writable, readable };
  static int timeout_millis(duration_t timeout);
};

} // cuti

#endif
