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

#ifndef CUTI_ASYNC_INBUF_HPP_
#define CUTI_ASYNC_INBUF_HPP_

#include "linkage.h"
#include "scheduler.hpp"

#include <cassert>
#include <cstddef>
#include <string>

namespace cuti
{

/*
 * Abstract asynchronous input buffer.
 */
struct CUTI_ABI async_inbuf_t
{
  static int constexpr eof = std::char_traits<char>::eof();
  
  explicit async_inbuf_t(std::size_t bufsize);

  async_inbuf_t(async_inbuf_t const&) = delete;
  async_inbuf_t& operator=(async_inbuf_t const&) = delete;

  /*
   * Tells if the buffer is currently readable.
   */
  bool readable() const
  {
    return read_ptr_ != limit_ || eof_seen_;
  }

  /*
   * Returns the buffer's error status: either 0 (OK) or a system
   * error code for the first error encountered.
   */
  int error_status() const
  {
    return error_status_;
  }

  /*
   * Returns the current input character, or eof at end of stream or
   * error.
   * PRE: this->readable().
   */
  int peek() const
  {
    assert(this->readable());
    return read_ptr_ != limit_ ?
      std::char_traits<char>::to_int_type(*read_ptr_) :
      eof;
  }

  /*
   * Moves to the next input character.
   * PRE: this->readable().
   */
  void skip()
  {
    assert(this->readable());
    if(read_ptr_ != limit_)
    {
      ++read_ptr_;
    }
  }

  /*
   * Extracts at most last - first input characters, returning a
   * pointer to the end of the extracted range, or first on end of
   * stream or error.
   * PRE: this->readable().
   */
  char* read(char* first, char const* last);

  /*
   * Schedules a callback for when *this is readable, returning any
   * previously scheduled callback. The scheduler must stay alive
   * while the callback is pending.
   */
  callback_t call_when_readable(scheduler_t& scheduler, callback_t callback);

  /*
   * Cancels and returns any previously scheduled callback.  No effect
   * if there is no pending callback.
   */
  callback_t cancel_when_readable() noexcept;
 
  virtual ~async_inbuf_t();

private :
  void on_readable_now();
  void on_derived_readable();

private :
  virtual cancellation_ticket_t
  do_call_when_readable(scheduler_t& scheduler, callback_t callback) = 0;

  virtual int
  do_read(char* first, char const* last, char*& next) = 0;

private :
  char* const buf_;
  char* const end_buf_;
  char* read_ptr_;
  char* limit_;

  bool eof_seen_;
  int error_status_;

  scheduler_t* scheduler_;
  cancellation_ticket_t readable_ticket_;
  callback_t callback_;
};

} // cuti

#endif
