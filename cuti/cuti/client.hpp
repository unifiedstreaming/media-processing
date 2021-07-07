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

#ifndef CUTI_CLIENT_HPP_
#define CUTI_CLIENT_HPP_

#include "linkage.h"
#include "cancellation_ticket.hpp"
#include "callback.hpp"

namespace cuti
{

struct scheduler_t;

/*
 * Abstract base class for a connected client.
 */
struct CUTI_ABI client_t
{
  client_t();

  client_t(client_t const&) = delete;
  client_t& operator=(client_t const&) = delete;

  /*
   * See tcp_connection.hpp for details.
   */
  virtual cancellation_ticket_t call_when_readable(
    scheduler_t& scheduler, callback_t callback) = 0;

  /*
   * Called when readable; returns true if more input is expected,
   * false when done.
   */
  virtual bool on_readable() = 0;
  
  virtual ~client_t();
};

} // cuti

#endif
