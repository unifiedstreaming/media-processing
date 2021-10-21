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

#ifndef CUTI_NB_SOURCE_HPP_
#define CUTI_NB_SOURCE_HPP_

#include "callback.hpp"
#include "cancellation_ticket.hpp"
#include "linkage.h"

namespace cuti
{

struct scheduler_t;

/*
 * Non-blocking character source interface
 */
struct CUTI_ABI nb_source_t
{
  nb_source_t()
  { }

  nb_source_t(nb_source_t const&) = delete;
  nb_source_t& operator=(nb_source_t const&) = delete;

  /*
   * Tries to read available input, storing it in [first, last>.  next
   * is set to either the end of the range that was read (which is
   * first on EOF), or to nullptr if the call would block.
   * Returns 0 on success; on error, next is set to first and a system
   * error code is returned.  Please note that hitting EOF or refusing
   * to block is not an error.
   */
  virtual int read(char* first, char const* last, char*& next) = 0;
  
  /*
   * Requests a one-time callback for when the source is detected to
   * be readable, returning a cancellation ticket that may be used to
   * cancel the callback by calling scheduler.cancel().
   */
  virtual cancellation_ticket_t call_when_readable(
    scheduler_t& scheduler, callback_t callback) = 0;

  /*
   * Returns a description for this source.
   */
  virtual char const* description() const noexcept = 0;
   
  virtual ~nb_source_t();
};

} // cuti

#endif
