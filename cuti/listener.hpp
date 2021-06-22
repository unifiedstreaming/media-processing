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

#ifndef CUTI_LISTENER_HPP_
#define CUTI_LISTENER_HPP_

#include "client.hpp"
#include "linkage.h"
#include "scheduler.hpp"

#include <memory>

namespace cuti
{

/*
 * Abstract base class listening for connecting clients.
 */
struct CUTI_ABI listener_t
{
  listener_t();

  listener_t(listener_t const&) = delete;
  listener_t& operator=(listener_t const&) = delete;

  /*
   * See tcp_acceptor.hpp for details.
   */
  virtual cancellation_ticket_t call_when_ready(
    scheduler_t& scheduler, callback_t callback) = 0;

  virtual std::unique_ptr<client_t> on_ready() = 0;

  virtual ~listener_t();
};

} // cuti

#endif
