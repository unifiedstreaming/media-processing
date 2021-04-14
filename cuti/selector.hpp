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

#include "basic_callback.hpp"
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
   * Registers a callback for when fd is writable.  Returns a
   * registration ticket, valid until either (1) the callback is
   * selected, or (2) the ticket is canceled.  Call this function
   * again if you want another callback.
   */
  virtual int call_when_writable(int fd, basic_callback_t callback) = 0;
  
  /*
   * Cancels a callback registered with call_when_writable(),
   * returning the registered callback.
   */
  virtual basic_callback_t cancel_when_writable(int ticket) = 0;

  /*
   * Registers a callback for when fd is readable.  Returns a
   * registration ticket, valid until either (1) the callback is
   * selected, or (2) the ticket is canceled.  Call this function
   * again if you want another callback.
   */
  virtual int call_when_readable(int fd, basic_callback_t callback) = 0;
  
  /*
   * Cancels a callback registered with call_when_readable(),
   * returning the registered callback.
   */
  virtual basic_callback_t cancel_when_readable(int ticket) = 0;

  /*
   * Returns true if there are no registered callbacks, and false if
   * there are.
   */
  virtual bool empty() const = 0;

  /*
   * Waits for for one of the registered events to occur, returning
   * the callback of the event detected first.  The selector must not
   * be empty.
   */
  basic_callback_t select()
  {
    return this->select(std::chrono::milliseconds(-1));
  }

  /*
   * Waits for at most timeout milliseconds for one of the registered
   * events to occur, returning either an empty callback if no event
   * was detected, or the non-empty callback of the event detected
   * first.  This function does not block if timeout is 0; a negative
   * timeout stands for no timeout and is only valid for a non-empty
   * selector.
   */
  virtual basic_callback_t select(std::chrono::milliseconds timeout) = 0;

  virtual ~selector_t();
};

} // cuti

#endif
