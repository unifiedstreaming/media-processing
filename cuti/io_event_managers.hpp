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

#ifndef CUTI_IO_EVENT_MANAGERS_HPP_
#define CUTI_IO_EVENT_MANAGERS_HPP_

#include "event_manager.hpp"
#include "io_scheduler.hpp"
#include "linkage.h"
#include "tcp_socket.hpp"

#include <utility>

namespace cuti
{

/*
 * This event adapter defines the 'tcp socket writable' event
 */
struct CUTI_ABI writable_event_adapter_t
{
  using scheduler_t = io_scheduler_t;

  explicit writable_event_adapter_t(tcp_socket_t& socket)
  : socket_(socket)
  { }

  template<typename F>
  int make_ticket(F&& callback, io_scheduler_t& scheduler)
  {
    return socket_.call_when_writable(std::forward<F>(callback), scheduler);
  }

  void cancel_ticket(io_scheduler_t& scheduler, int ticket) noexcept
  {
    scheduler.cancel_when_writable(ticket);
  }
    
private :
  tcp_socket_t& socket_;
};

/*
 * This event adapter defines the 'tcp socket readable' event
 */
struct CUTI_ABI readable_event_adapter_t
{
  using scheduler_t = io_scheduler_t;

  explicit readable_event_adapter_t(tcp_socket_t& socket)
  : socket_(socket)
  { }

  template<typename F>
  int make_ticket(F&& callback, io_scheduler_t& scheduler)
  {
    return socket_.call_when_readable(std::forward<F>(callback), scheduler);
  }

  void cancel_ticket(io_scheduler_t& scheduler, int ticket) noexcept
  {
    scheduler.cancel_when_readable(ticket);
  }
    
private :
  tcp_socket_t& socket_;
};

using writable_event_manager_t = event_manager_t<writable_event_adapter_t>;
using readable_event_manager_t = event_manager_t<readable_event_adapter_t>;

} // cuti

#endif
