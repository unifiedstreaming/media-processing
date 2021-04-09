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
   * Registers a callback for when fd becomes writable, returning a
   * registration id.
   */
  virtual int call_when_writable(int fd, void (*callback)(void*), void* arg)
    = 0;
  
  /*
   * Cancels a callback registered with call_when_writable().
   */
  virtual void cancel_when_writable(int registration_id) = 0;

  /*
   * Registers a callback for when fd becomes readable, returning a
   * registration id.
   */
  virtual int call_when_readable(int fd, void (*callback)(void*), void* arg)
    = 0;
  
  /*
   * Cancels a callback registered with call_when_readable().
   */
  virtual void cancel_when_readable(int registration_id) = 0;

  /*
   * Returns true if there are no registered callbacks, false otherwise.
   */
  virtual bool empty() const = 0;

  /*
   * Waits until limit for any of the registered events to occur.  If
   * any event is detected before limit is reached, its registered
   * callback is invoked, and true is returned; otherwise, false is
   * returned.
   *
   * The callback is invoked with the arg value passed to
   * call_when_*().  When the callback is invoked, the corresponding
   * event has already been unregistered; invoke call_when_*() again
   * if you're interested in further callbacks for the same kind of
   * event.
   */
  virtual bool progress(std::chrono::system_clock::time_point limit) = 0;

  /*
   * Destroys the selector.  Please note that destroying a selector
   * that is not empty leads to undefined behavior.
   */
  virtual ~selector_t();
};

} // cuti

#endif
