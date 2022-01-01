/*
 * Copyright (C) 2021-2022 CodeShop B.V.
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

#ifndef CUTI_CIRCULAR_BUFFER_HPP_
#define CUTI_CIRCULAR_BUFFER_HPP_

#include "linkage.h"

#include <cassert>
#include <cstddef>

namespace cuti
{

/*
 * circular_buffer_t is a special-purpose fixed-capacity buffer
 * intended for network communication.
 *
 * A circular buffer contains an uninitialized slack area, which may
 * be used for receiving data, and an initialized data area, which may
 * be used for sending data.  If the circular buffer has a non-zero
 * capacity, then, at any point in time, at least one of these areas
 * is non-empty.
 * 
 * The slack area may be viewed as scribble memory; it is write- and
 * read-accessible.  Once properly initialized (e.g., after network
 * data was received into it), some initial part of the slack area may
 * be appended to the data area by the circular buffer's push_back()
 * method.
 *
 * The data area is strictly read-only; when no longer needed (e.g.,
 * after it was sent out over the network) some initial part of the
 * data area may be recycled to the slack area by the circular
 * buffer's pop_front() method.
 *
 * Both areas wrap around if needed, and so consist of either one or
 * two contiguous blocks of memory.  The circular buffer keeps track
 * of that, but its API only provides direct access to the first
 * contiguous block of either area.
 */

struct CUTI_ABI circular_buffer_t
{
  /*
   * Constructs a zero-capacity circular buffer with no usable
   * slack or data area.
   */
  circular_buffer_t() noexcept;

  /*
   * Constructs a circular buffer.  In its initial state, all of the
   * buffer's capacity is used for its slack area, which is not
   * initialized.
   */
  explicit circular_buffer_t(std::size_t capacity);

  /*
   * Copy-constructs a circular buffer.  Please note that the slack
   * area is not considered part of the state of the source buffer:
   * only the data area is copied, and the slack area in the
   * destination buffer is not initialized.
   */
  circular_buffer_t(circular_buffer_t const& rhs);

  /*
   * Move-constructs a circular buffer, transferring both the slack
   * and data areas.  The source buffer's capacity becomes zero.
   */
  circular_buffer_t(circular_buffer_t&& rhs) noexcept;

  /*
   * Copy-assigns a circular buffer.  Please note that the slack area
   * is not considered part of the state of the source buffer: only
   * the data area is copied, and the slack area in the destination
   * buffer is not initialized.
   */
  circular_buffer_t& operator=(circular_buffer_t const& rhs);

  /*
   * Move-assigns a circular buffer, transferring both the slack and
   * data areas.  The source buffer's capacity becomes zero, except
   * that if this == &rhs, there is no effect.
   */
  circular_buffer_t& operator=(circular_buffer_t&& rhs) noexcept;
  
  /*
   * Swaps two circular buffers, exchanging their slack and data
   * areas.
   */
  void swap(circular_buffer_t& that) noexcept;

  /*
   * Returns the buffer's capacity.
   */
  std::size_t capacity() const noexcept
  { return end_ - buf_; }

  /*
   * Returns the total size of the slack area, including any second
   * contiguous slack memory block.
   */
  std::size_t total_slack_size() const noexcept
  {
    return empty_ ? end_ - buf_ :
           slack_ <= data_ ? data_ - slack_ :
           (end_ - slack_) + (data_ - buf_);
  }

  /*
   * Returns the total size of the data area, including any second
   * contiguous data memory block.
   */
  std::size_t total_data_size() const noexcept
  {
    return empty_ ? 0 :
           data_ < slack_ ? slack_ - data_ :
           (end_ - data_) + (slack_ - buf_);
  }

  /*
   * Sets the buffer's capacity; no effect if capacity < total_data_size().
   */
  void reserve(std::size_t capacity);

  /*
   * Tells if the buffer has space in its slack area.
   */
  bool has_slack() const noexcept
  { return empty_ ? buf_ != end_ : slack_ != data_; }

  /*
   * Returns the beginning of the first contiguous slack memory block.
   */
  char const* begin_slack() const noexcept
  { return slack_; }

  char* begin_slack() noexcept
  { return slack_; }

  /*
   * Returns the end of the first contiguous slack memory block.
   */
  char const* end_slack() const noexcept
  { return empty_ || slack_ > data_ ? end_ : data_; }

  char* end_slack() noexcept
  { return empty_ || slack_ > data_ ? end_ : data_;  }

  /*
   * Moves the range [begin_slack(), until> to the end of the data
   * area; until must be <= end_slack().
   */
  void push_back(char* until) noexcept
  {
    assert(until >= begin_slack());
    assert(until <= end_slack());

    if(until != slack_)
    {
      empty_ = false;
      slack_ = until != end_ ? until : buf_;
    }
  }

  /*
   * Tells if there is data in the buffer's data area.
   */
  bool has_data() const noexcept
  { return !empty_; }

  /*
   * Returns the beginning of the first contiguous data memory block.
   */
  char const* begin_data() const noexcept
  { return data_; }

  /*
   * Returns the end of the first contiguous data memory block.
   */
  char const* end_data() const noexcept
  { return empty_ || data_ < slack_ ? slack_ : end_; }

  /*
   * Moves the range [begin_data(), until> to the end of the slack
   * area; until must be <= end_data().  Resets the buffer to its
   * initial state if the data area becomes empty.
   */
  void pop_front(char const* until) noexcept
  {
    assert(until >= begin_data());
    assert(until <= end_data());

    if(until != data_)
    {
      data_ = until != end_ ? const_cast<char*>(until) : buf_;
      if(data_ == slack_)
      {
        empty_ = true;
        data_ = buf_;
        slack_ = buf_;
      }
    }
  }
    
  ~circular_buffer_t();

private :
  bool empty_;
  char* buf_;
  char* data_;
  char* slack_;
  char* end_;
};

CUTI_ABI
void swap(circular_buffer_t& b1, circular_buffer_t& b2) noexcept;

} // cuti

#endif
