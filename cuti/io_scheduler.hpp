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

#include "callback.hpp"
#include "linkage.h"
#include "socket_nifty.hpp"

#include <utility>
#include <memory>

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
   * Schedules a one-time callback for when fd is ready for writing;
   * the callback must be != nullptr.  Returns a non-negative
   * cancellation ticket that may be used to cancel the callback
   * before it is invoked.  Call this function again if you want
   * another callback.
   */
  virtual int call_when_writable(int fd, callback_t callback) = 0;

  /*
   * Cancels a callback scheduled with call_when_writable().
   */
  virtual void cancel_when_writable(int cancellation_ticket) noexcept = 0;

  /*
   * Schedules a one-time callback for when fd is ready for reading;
   * the callback must be != nullptr.  Returns a non-negative
   * cancellation ticket that may be used to cancel the callback
   * before it is invoked.  Call this function again if you want
   * another callback.
   */
  virtual int call_when_readable(int fd, callback_t callback) = 0;

  /*
   * Cancels a callback scheduled with call_when_readable().
   */
  virtual void cancel_when_readable(int cancellation_ticket) noexcept = 0;

  virtual ~io_scheduler_t();
};

// SSTS: static start takes shared
template<typename IOHandler, typename... Args>
auto start_io_handler(io_scheduler_t& scheduler, Args&&... args)
{
  auto handler = std::make_shared<IOHandler>(std::forward<Args>(args)...);
  return IOHandler::start(handler, scheduler);
}
    
} // cuti

#endif
