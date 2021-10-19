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

#include "throughput_tracker.hpp"

#include <cassert>

namespace cuti
{

throughput_tracker_t::throughput_tracker_t(
  std::size_t min_bytes_per_tick,
  unsigned int slow_ticks_limit,
  duration_t tick_length)
: min_bytes_per_tick_(min_bytes_per_tick)
, slow_ticks_limit_(slow_ticks_limit)
, tick_length_((assert(tick_length > duration_t::zero()), tick_length))
, next_tick_(cuti_clock_t::now() + tick_length_)
, current_tick_bytes_(0)
, n_slow_ticks_(0)
{ }

void throughput_tracker_t::record_transfer(std::size_t n_bytes)
{
  this->update();

  if(n_bytes < min_bytes_per_tick_ - current_tick_bytes_)
  {
    current_tick_bytes_ += n_bytes;
  }
  else
  {
    n_slow_ticks_ = 0;
    current_tick_bytes_ = min_bytes_per_tick_;
  }
}

bool throughput_tracker_t::is_low()
{
  this->update();

  return n_slow_ticks_ >= slow_ticks_limit_;
}
  
void throughput_tracker_t::update()
{
  time_point_t now = cuti_clock_t::now();
  while(next_tick_ <= now)
  {
    if(current_tick_bytes_ < min_bytes_per_tick_ &&
       n_slow_ticks_ < slow_ticks_limit_)
    {
       ++n_slow_ticks_;
    }

    current_tick_bytes_ = 0;
    next_tick_ += tick_length_;
  }
}
  
} // cuti
