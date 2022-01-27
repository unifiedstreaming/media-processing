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

#ifndef CUTI_BOUND_OUTBUF_HPP_
#define CUTI_BOUND_OUTBUF_HPP_

#include "callback.hpp"
#include "linkage.h"
#include "nb_outbuf.hpp"
#include "stack_marker.hpp"

#include <ostream>
#include <utility>

namespace cuti
{

struct scheduler_t;

/*
 * A scoping vehicle for managing the assocation between an nb_outbuf_t
 * and a scheduler.
 */
struct CUTI_ABI bound_outbuf_t
{
  static duration_t constexpr default_tick_length =
    nb_outbuf_t::default_tick_length;

  bound_outbuf_t(stack_marker_t& base_marker,
                 nb_outbuf_t& outbuf,
                 scheduler_t& scheduler)
  : base_marker_(base_marker)
  , outbuf_(outbuf)
  , scheduler_(scheduler)
  { }

  bound_outbuf_t(bound_outbuf_t const&) = delete;
  bound_outbuf_t& operator=(bound_outbuf_t const&) = delete;
  
  stack_marker_t const& base_marker() const
  {
    return base_marker_;
  }
  
  int error_status() const noexcept
  {
    return outbuf_.error_status();
  }

  bool writable() const
  {
    return outbuf_.writable();
  }

  void put(char c) const
  {
    outbuf_.put(c);
  }

  char const* write(char const* first, char const* last)
  {
    return outbuf_.write(first, last);
  }

  void start_flush()
  {
    outbuf_.start_flush();
  }

  void call_when_writable(callback_t callback)
  {
    outbuf_.call_when_writable(scheduler_, std::move(callback));
  }

  void cancel_when_writable() noexcept
  {
    outbuf_.cancel_when_writable();
  }

  void enable_throughput_checking(
    std::size_t min_bytes_per_tick,
    unsigned int low_ticks_limit,
    duration_t tick_length = default_tick_length)
  {
    outbuf_.enable_throughput_checking(
      min_bytes_per_tick, low_ticks_limit, tick_length);
  }

  void disable_throughput_checking()
  {
    outbuf_.disable_throughput_checking();
  }

  ~bound_outbuf_t()
  {
    this->cancel_when_writable();
    this->disable_throughput_checking();
  }

  friend std::ostream& operator<<(std::ostream& os, bound_outbuf_t& buf)
  {
    return os << buf.outbuf_;
  }

private :
  stack_marker_t& base_marker_;
  nb_outbuf_t& outbuf_;
  scheduler_t& scheduler_;
};

} // cuti

#endif
