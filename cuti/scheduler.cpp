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

#include "scheduler.hpp"

#include <cassert>

namespace cuti
{

scheduler_t::scheduler_t()
{ }

void scheduler_t::cancel(cancellation_ticket_t ticket) noexcept
{
  assert(!ticket.empty());

  switch(ticket.type())
  {
  case cancellation_ticket_t::type_t::alarm :
    this->do_cancel_alarm(ticket.id());
    break;
  case cancellation_ticket_t::type_t::writable :
    this->do_cancel_when_writable(ticket.id());
    break;
  case cancellation_ticket_t::type_t::readable :
    this->do_cancel_when_readable(ticket.id());
    break;
  default :
    assert(!"expected ticket type");
    break;
  }
}
    
scheduler_t::~scheduler_t()
{ }

} // namespace cuti
