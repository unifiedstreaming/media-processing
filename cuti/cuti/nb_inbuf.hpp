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

#ifndef CUTI_NB_INBUF_HPP_
#define CUTI_NB_INBUF_HPP_

#include "callback.hpp"
#include "eof.hpp"
#include "linkage.h"
#include "nb_source.hpp"
#include "nb_tickets_holder.hpp"

#include <cassert>
#include <cstddef>
#include <memory>
#include <string>

namespace cuti
{

struct logging_context_t;
struct scheduler_t;

struct CUTI_ABI nb_inbuf_t
{
  static std::size_t constexpr default_bufsize = 256 * 1024;

  nb_inbuf_t(logging_context_t& context,
             std::unique_ptr<nb_source_t> source,
             std::size_t bufsize = default_bufsize);

  nb_inbuf_t(nb_inbuf_t const&) = delete;
  nb_inbuf_t& operator=(nb_inbuf_t const&) = delete;

  /*
   * Returns a descriptive name for the buffer.
   */
  char const* name() const noexcept
  {
    return source_->name();
  }

  /*
   * Returns the buffer's error status, which is either 0 for no error
   * or a system error code.  The buffer's error status is sticky;
   * once in error, the buffer will consistently report EOF.
   */
  int error_status() const noexcept
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
   * first on EOF.
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

private :
  void on_already_readable(scheduler_t& scheduler);
  void on_source_readable(scheduler_t& scheduler);

private :
  logging_context_t& context_;

  std::unique_ptr<nb_source_t> source_;

  nb_tickets_holder_t<nb_inbuf_t, &nb_inbuf_t::on_already_readable>
    already_readable_holder_;
  nb_tickets_holder_t<nb_inbuf_t, &nb_inbuf_t::on_source_readable>
    source_readable_holder_;
  callback_t callback_;

  char* const buf_;
  char const* rp_;
  char const* ep_;
  char const* const ebuf_;

  bool at_eof_;
  int error_status_;
};

} // cuti

#endif
