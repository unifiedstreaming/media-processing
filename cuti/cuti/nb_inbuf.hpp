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

#ifndef CUTI_NB_INBUF_HPP_
#define CUTI_NB_INBUF_HPP_

#include "callback.hpp"
#include "cancellation_ticket.hpp"
#include "charclass.hpp"
#include "error_status.hpp"
#include "linkage.h"
#include "nb_source.hpp"
#include "stack_marker.hpp"
#include "throughput_checker.hpp"

#include <cassert>
#include <cstddef>
#include <iosfwd>
#include <memory>
#include <optional>
#include <string>

namespace cuti
{

struct scheduler_t;

struct CUTI_ABI nb_inbuf_t
{
  static std::size_t constexpr default_bufsize = 8 * 1024;

  nb_inbuf_t(std::unique_ptr<nb_source_t> source,
             std::size_t bufsize = default_bufsize);

  nb_inbuf_t(nb_inbuf_t const&) = delete;
  nb_inbuf_t& operator=(nb_inbuf_t const&) = delete;

  /*
   * Enable throughput checking, which is disabled by default.  See
   * throughput_checker.hpp for details.
   */
  void enable_throughput_checking(throughput_settings_t settings);

  /*
   * Disable throughput checking.
   */
  void disable_throughput_checking() noexcept;

  /*
   * Returns the buffer's error status.  The buffer's error status is
   * sticky; once in error, the buffer will consistently report EOF.
   */
  error_status_t error_status() const noexcept
  {
    return error_status_;
  }

  /*
   * Returns true if input is available or EOF has been seen, false
   * otherwise.
   */
  bool readable() const
  {
    return rp_ != ep_ || at_eof_;
  }

  /*
   * Returns the current input character or eof.
   * PRE: this->readable().
   */
  int peek() const
  {
    assert(this->readable());
    return rp_ != ep_ ? std::char_traits<char>::to_int_type(*rp_) : eof;
  }

  /*
   * Skips the current input character.
   * PRE: this->readable().
   */
  void skip()
  {
    assert(this->readable());
    if(rp_ != ep_)
    {
      ++rp_;
    }
  }

  /*
   * Reads up to last - first characters, storing them in the range
   * starting at first.
   * Returns the end of the range of the characters read, which is
   * first if the buffer is at eof.
   * PRE: this->readable().
   */
  char* read(char* first, char const* last);

  /*
   * Schedules a callback for when the buffer is detected to be
   * readable, canceling any previously requested callback.
   * The scheduler must stay alive while the callback is pending.
   */
  void call_when_readable(scheduler_t& scheduler, callback_t callback);

  /*
   * Cancels any pending callback; no effect is there is no pending
   * callback.
   */
  void cancel_when_readable() noexcept;

  ~nb_inbuf_t();

  friend std::ostream& operator<<(std::ostream& os, nb_inbuf_t const& buf)
  {
    buf.source_->print(os);
    return os;
  }
    
private :
  void on_already_readable(stack_marker_t& base_marker);
  void on_source_readable(stack_marker_t& base_marker);
  void on_next_tick(stack_marker_t& base_marker);

private :
  std::unique_ptr<nb_source_t> source_;
  std::optional<throughput_checker_t<>> checker_;

  cancellation_ticket_t readable_ticket_;
  cancellation_ticket_t alarm_ticket_;
  scheduler_t* scheduler_;
  callback_t callback_;

  char* const buf_;
  char const* rp_;
  char const* ep_;
  char const* const ebuf_;

  bool at_eof_;
  error_status_t error_status_;
};

} // cuti

#endif
