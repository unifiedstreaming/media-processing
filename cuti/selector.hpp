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

#include "linkage.h"
#include "socket_nifty.hpp"

#include <chrono>

namespace cuti
{

/*
 * Abstract selector interface
 */
struct CUTI_ABI selector_t
{
  selector_t()
  { }

  selector_t(selector_t const&) = delete;
  selector_t& operator=(selector_t const&) = delete;

  /*
   * Registers a callback for when fd becomes writable.  Returns a
   * registration ticket, valid until either (1) the callback is
   * invoked, or (2) the ticket is canceled.  Call this function again
   * if you want another callback.
   */
  virtual int call_when_writable(int fd, void (*callback)(void*), void* arg)
    = 0;
  
  /*
   * Cancels a callback registered with call_when_writable().
   */
  virtual void cancel_when_writable(int ticket) = 0;

  /*
   * Registers a callback for when fd becomes readable.  Returns a
   * registration ticket, valid until either (1) the callback is
   * invoked, or (2) the ticket is canceled.  Call this function again
   * if you want another callback.
   */
  virtual int call_when_readable(int fd, void (*callback)(void*), void* arg)
    = 0;
  
  /*
   * Cancels a callback registered with call_when_readable().
   */
  virtual void cancel_when_readable(int ticket) = 0;

  /*
   * Returns true if there are no registered callbacks, false if there
   * are.
   */
  virtual bool empty() const = 0;

  /*
   * Waits until limit for any of the registered events to occur.  If
   * any event is detected before limit is reached, its registered
   * callback is invoked with the arg value passed to
   * call_when_{writable,readable}(), and true is returned; otherwise,
   * false is returned.
   */
  virtual bool progress(std::chrono::system_clock::time_point limit) = 0;

  /*
   * Destroys the selector.
   * Please note: destroying a selector while there are registered
   * callbacks (!empty()) results in undefined behavior.
   */
  virtual ~selector_t();
};

} // cuti

#endif
