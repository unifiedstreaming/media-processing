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

#ifndef CUTI_BOUND_INBUF_HPP_
#define CUTI_BOUND_INBUF_HPP_

#include "callback.hpp"
#include "linkage.h"
#include "nb_inbuf.hpp"
#include "stack_marker.hpp"

#include <ostream>
#include <utility>

namespace cuti
{

struct scheduler_t;

/*
 * A scoping vehicle for managing the assocation between an nb_inbuf_t
 * and a scheduler.
 */
struct CUTI_ABI bound_inbuf_t
{
  bound_inbuf_t(stack_marker_t& base_marker,
                nb_inbuf_t& inbuf,
                scheduler_t& scheduler)
  : base_marker_(base_marker)
  , inbuf_(inbuf)
  , scheduler_(scheduler)
  { }

  bound_inbuf_t(bound_inbuf_t const&) = delete;
  bound_inbuf_t& operator=(bound_inbuf_t const&) = delete;

  stack_marker_t const& base_marker() const
  {
    return base_marker_;
  }
  
  error_status_t error_status() const noexcept
  {
    return inbuf_.error_status();
  }

  bool readable() const
  {
    return inbuf_.readable();
  }

  int peek() const
  {
    return inbuf_.peek();
  }

  void skip()
  {
    inbuf_.skip();
  }

  char* read(char* first, char const* last)
  {
    return inbuf_.read(first, last);
  }

  void call_when_readable(callback_t callback)
  {
    inbuf_.call_when_readable(scheduler_, std::move(callback));
  }

  void cancel_when_readable() noexcept
  {
    inbuf_.cancel_when_readable();
  }

  void enable_throughput_checking(throughput_settings_t settings)
  {
    inbuf_.enable_throughput_checking(std::move(settings));
  }

  void disable_throughput_checking()
  {
    inbuf_.disable_throughput_checking();
  }

  ~bound_inbuf_t()
  {
    this->cancel_when_readable();
    this->disable_throughput_checking();
  }

  friend std::ostream& operator<<(std::ostream& os, bound_inbuf_t& buf)
  {
    return os << buf.inbuf_;
  }

private :
  stack_marker_t& base_marker_;
  nb_inbuf_t& inbuf_;
  scheduler_t& scheduler_;
};

} // cuti

#endif
