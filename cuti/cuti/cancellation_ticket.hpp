/*
 * Copyright (C) 2021-2024 CodeShop B.V.
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

#ifndef CUTI_CANCELLATION_TICKET_HPP_
#define CUTI_CANCELLATION_TICKET_HPP_

#include "linkage.h"

namespace cuti
{

struct scheduler_t;

/*
 * Cancellation tickets are used to cancel a previously scheduled
 * scheduler callback before it is invoked.  See scheduler.hpp for
 * details.
 * 
 * Please note: a cancellation ticket is only valid until the callback
 * is selected.  Any attempt to cancel a callback during or after its
 * invocation leads to undefined behavior.
 */
struct CUTI_ABI cancellation_ticket_t
{
  /*
   * Constructs an empty cancellation ticket.
   */
  cancellation_ticket_t() noexcept
  : type_(type_t::empty)
  , id_(-1)
  { }

  /*
   * Tells if the ticket is empty.  Scheduling a callback returns a
   * non-empty cancellation ticket.
   */
  bool empty() const noexcept
  { return type_ == type_t::empty; }

  /*
   * Sets the ticket to the empty state.
   */
  void clear() noexcept
  { *this = cancellation_ticket_t(); }

private :
  friend struct scheduler_t;

  enum class type_t { empty, alarm, writable, readable };

  explicit cancellation_ticket_t(type_t type, int id) noexcept
  : type_(type)
  , id_(id)
  { }

  type_t type() const noexcept
  { return type_; }

  int id() const noexcept
  { return id_; }

private :
  type_t type_;
  int id_;
};

} // cuti

#endif
