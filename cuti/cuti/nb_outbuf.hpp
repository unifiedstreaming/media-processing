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

#ifndef CUTI_NB_OUTBUF_HPP_
#define CUTI_NB_OUTBUF_HPP_

#include "callback.hpp"
#include "linkage.h"
#include "nb_sink.hpp"
#include "nb_tickets_holder.hpp"
#include "throughput_checker.hpp"

#include <cassert>
#include <cstddef>
#include <memory>
#include <iosfwd>
#include <optional>

namespace cuti
{

struct logging_context_t;
struct scheduler_t;

struct CUTI_ABI nb_outbuf_t
{
  static std::size_t constexpr default_bufsize = 256 * 1024;

  nb_outbuf_t(logging_context_t& context,
              std::unique_ptr<nb_sink_t> sink,
              std::size_t bufsize = default_bufsize);

  nb_outbuf_t(nb_outbuf_t const&) = delete;
  nb_outbuf_t& operator=(nb_outbuf_t const&) = delete;

  /*
   * Enable throughput checking, which is disabled by default.  See
   * throughput_checker.hpp for details.
   */
  void enable_throughput_checking(std::size_t min_bytes_per_tick,
                                  unsigned int low_ticks_limit,
                                  duration_t tick_length = seconds_t(1));

  /*
   * Disable throughput checking.
   */
  void disable_throughput_checking();  

  /*
   * Returns the buffer's error status, which is either 0 for no error
   * or a system error code.  The buffer's error status is sticky.
   */
  int error_status() const noexcept
  {
    return error_status_;
  }

  /*
   * Returns true if buffer space is available.
   */
  bool writable() const
  {
    return wp_ != limit_;
  }

  /*
   * Writes a single character.
   * PRE: this->writable()
   */
  void put(char c)
  {
    assert(this->writable());

    if(error_status_ == 0)
    {
      *wp_ = c;
      ++wp_;
    }
  }

  /*
   * Writes up to last - first characters from the range starting at
   * first.
   * Returns a pointer to next character to write.
   * PRE: this->writable()
   */
  char const* write(char const* first, char const* last);

  /*
   * Enters flushing mode.  The buffer becomes writable again when
   * all characters have been flushed.
   */
  void start_flush()
  {
    if(rp_ != wp_)
    {
      limit_ = wp_;
    }
  }

  /*
   * Schedules a callback for when the buffer is detected to be
   * writable, canceling any previously requested callback.
   * The scheduler must remain alive while the callback is pending.
   */
  void call_when_writable(scheduler_t& scheduler, callback_t callback);

  /*
   * Cancels any pending callback; no effect is there is no pending
   * callback.
   */
  void cancel_when_writable() noexcept;

  ~nb_outbuf_t();

  friend std::ostream& operator<<(std::ostream& os, nb_outbuf_t const& buf)
  {
    buf.sink_->print(os);
    return os;
  }
    
private :
  void check_writable(scheduler_t& scheduler);

private :
  logging_context_t& context_;

  std::unique_ptr<nb_sink_t> sink_;

  nb_tickets_holder_t<nb_outbuf_t, &nb_outbuf_t::check_writable> holder_;
  callback_t callback_;
  std::optional<throughput_checker_t<>> checker_;

  char* const buf_;
  char const* rp_;
  char* wp_;
  char const* limit_;
  char const* const ebuf_;
  
  int error_status_;
};

} // cuti

#endif
