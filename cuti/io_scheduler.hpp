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

#ifndef CUTI_IO_SCHEDULER_HPP_
#define CUTI_IO_SCHEDULER_HPP_

#include "basic_callback.hpp"
#include "linkage.h"
#include "socket_nifty.hpp"

namespace cuti
{

/*
 * Abstract I/O event scheduler interface
 */
struct CUTI_ABI io_scheduler_t
{
  io_scheduler_t()
  { }

  io_scheduler_t(io_scheduler_t const&) = delete;
  io_scheduler_t& operator=(io_scheduler_t const&) = delete;

  /*
   * Registers a callback for when fd is ready for writing; the
   * callback must not be empty.  Returns a registration ticket, valid
   * until either (1) the ticket is canceled, or (2) callback is
   * invoked.  Call this function again if you want another callback.
   */
  virtual int call_when_writable(int fd, basic_callback_t callback) = 0;
  
  /*
   * Cancels a callback registered with call_when_writable(),
   * returning the registered callback.
   */
  virtual basic_callback_t cancel_when_writable(int ticket) = 0;

  /*
   * Registers a callback for when fd is ready for reading; the
   * callback must not be empty.  Returns a registration ticket, valid
   * until either (1) the ticket is canceled, or (2) callback is
   * invoked.  Call this function again if you want another callback.
   */
  virtual int call_when_readable(int fd, basic_callback_t callback) = 0;
  
  /*
   * Cancels a callback registered with call_when_readable(),
   * returning the registered callback.
   */
  virtual basic_callback_t cancel_when_readable(int ticket) = 0;

  virtual ~io_scheduler_t();
};

} // cuti

#endif
