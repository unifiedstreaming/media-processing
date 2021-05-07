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

#ifndef CUTI_CANCELLATION_TICKET_HPP_
#define CUTI_CANCELLATION_TICKET_HPP_

template<typename Scheduler, typename Tag>
struct cancellation_ticket_t
{
  /*
   * Constructs an empty cancellation ticket
   */
  cancellation_ticket_t() noexcept
  : id_(-1)
  { }

  /*
   * Tells if the ticket is empty.  Scheduling a callback returns a
   * non-empty cancellation ticket, but even non-empty tickets become
   * invalid when the callback is invoked.
   */
  bool empty() const noexcept
  { return id_ == -1; }

  /*
   * Sets the ticket to the empty state.
   */
  void clear() noexcept
  { id_ = -1; }

private :
  friend Scheduler;

  explicit cancellation_ticket_t(int id) noexcept
  : id_(id)
  { }

  int id() const noexcept
  { return id_; }

private :
  int id_;
};

#endif
