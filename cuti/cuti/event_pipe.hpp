/*
 * Copyright (C) 2022-2024 CodeShop B.V.
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

#ifndef CUTI_EVENT_PIPE_HPP_
#define CUTI_EVENT_PIPE_HPP_

#include "callback.hpp"
#include "charclass.hpp"
#include "cancellation_ticket.hpp"
#include "linkage.h"
#include "socket_nifty.hpp"

#include <memory>
#include <optional>
#include <utility>

namespace cuti
{

struct scheduler_t;

struct CUTI_ABI event_pipe_reader_t
{
  event_pipe_reader_t()
  { }

  event_pipe_reader_t(event_pipe_reader_t const&) = delete;
  event_pipe_reader_t& operator=(event_pipe_reader_t const&) = delete;

  /*
   * Blocking mode control; the default is blocking.
   */
  virtual void set_blocking() = 0;
  virtual void set_nonblocking() = 0;

  /*
   * Tries to read from the event pipe.  Returns a non-empty optional
   * on success (either an unsigned byte value passed to the connected
   * writer or eof if the writer was deleted), or an empty optional if
   * the call would block (non-blocking mode only).
   */
  virtual std::optional<int> read() = 0;

  /*
   * Readability reporter; see scheduler.hpp for detailed semantics.
   * A callback can be canceled by calling cancel() directly on the
   * scheduler.
   */
  virtual cancellation_ticket_t call_when_readable(
    scheduler_t& scheduler, callback_t callback) const = 0;

  virtual ~event_pipe_reader_t()
  { }
};

struct CUTI_ABI event_pipe_writer_t
{
  event_pipe_writer_t()
  { }

  event_pipe_writer_t(event_pipe_writer_t const&) = delete;
  event_pipe_writer_t& operator=(event_pipe_writer_t const&) = delete;

  /*
   * Blocking mode control; the default is blocking.
   */
  virtual void set_blocking() = 0;
  virtual void set_nonblocking() = 0;

  /*
   * Tries to write a single byte to the event pipe.  Returns true on
   * success, and false if the call would block (non-blocking mode
   * only).
   * An attempt to write after deleting the connected event pipe
   * reader results in undefined behavior.
   */
  virtual bool write(unsigned char event) = 0;

  /*
   * Writability reporter; see scheduler.hpp for detailed semantics.
   * A callback can be canceled by calling cancel() directly on the
   * scheduler.
   */
  virtual cancellation_ticket_t call_when_writable(
    scheduler_t& scheduler, callback_t callback) const = 0;

  virtual ~event_pipe_writer_t()
  { }
};

CUTI_ABI
std::pair<std::unique_ptr<event_pipe_reader_t>,
          std::unique_ptr<event_pipe_writer_t>>
make_event_pipe();

} // cuti

#endif
