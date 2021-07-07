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

#ifndef CUTI_ASYNC_OUTBUF_HPP_
#define CUTI_ASYNC_OUTBUF_HPP_

#include "async_output.hpp"
#include "linkage.h"
#include "ticket_holder.hpp"

#include <cassert>
#include <cstddef>
#include <memory>
#include <vector>

namespace cuti
{

/*
 * Asynchronous output buffer.
 */
struct CUTI_ABI async_outbuf_t
{
  static std::size_t constexpr default_bufsize = 256 * 1024;
  
  /*
   * Construct with default_bufsize.
   */
  explicit async_outbuf_t(std::unique_ptr<async_output_t> output);

  /*
   * Construct with the specified bufsize.
   */
  async_outbuf_t(std::unique_ptr<async_output_t> output, std::size_t bufsize);

  async_outbuf_t(async_outbuf_t const&) = delete;
  async_outbuf_t& operator=(async_outbuf_t const&) = delete;

  /*
   * Tells if the buffer is currently writable.
   */
  bool writable() const
  {
    return write_ptr_ != limit_;
  }

  /*
   * Returns the buffer's error status: either 0 (OK) or a system
   * error code for the first error encountered.
   */
  int error_status() const noexcept;

  /*
   * Writes a single character.
   * PRE: this->writable().
   */
  void put(char c)
  {
    assert(this->writable());
    *write_ptr_ = c;
    ++write_ptr_;
  }

  /*
   * Writes at most last - first characters, returning a pointer to
   * the next character to write.
   * PRE: this->writable().
   */
  char const* write(char const* first, char const* last);

  /*
   * Initiates a flush. The buffer will become writable again when all
   * pending output has been sent.
   * PRE: this->writable().
   */
  void start_flush()
  {
    assert(this->writable());
    limit_ = write_ptr_;
  }
   
  /*
   * Schedules a callback for when *this is writable. The scheduler
   * must stay alive while the callback is pending.
   */
  void call_when_writable(scheduler_t& scheduler, callback_t callback);

  /*
   * Cancels any previously scheduled callback.  No effect if there is
   * no pending callback.
   */
  void cancel_when_writable() noexcept;
 
  /*
   * Destroys the buffer.  Please note that destroying a buffer that has
   * not been flushed will probably lead to data loss.
   */
  ~async_outbuf_t();

private :
  void on_writable_now();
  void on_output_writable(scheduler_t& scheduler);

private :
  std::unique_ptr<async_output_t> output_;

  std::vector<char> buf_;
  char* read_ptr_;
  char* write_ptr_;
  char* limit_;

  ticket_holder_t writable_now_holder_;
  callback_t user_callback_;
};

} // cuti

#endif
