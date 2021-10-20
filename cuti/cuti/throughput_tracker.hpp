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

#ifndef CUTI_THROUGHPUT_TRACKER_HPP_
#define CUTI_THROUGHPUT_TRACKER_HPP_

#include "clock_object.hpp"
#include "chrono_types.hpp"

#include <cassert>
#include <cstddef>
#include <utility>

namespace cuti
{

template<typename ClockObjectT = default_clock_object_t>
struct throughput_tracker_t
{
  /*
   * Constructs a throughput tracker.  The throughput is considered to
   * be low if less than min_bytes_per_tick were transferred for at
   * least low_ticks_limit ticks.
   */
  throughput_tracker_t(std::size_t min_bytes_per_tick,
                       unsigned int low_ticks_limit,
                       duration_t tick_length,
                       ClockObjectT clock = ClockObjectT{})
  : clock_(std::move(clock))
  , min_bytes_per_tick_(min_bytes_per_tick)
  , low_ticks_limit_(low_ticks_limit)
  , tick_length_((assert(tick_length > duration_t::zero()), tick_length))
  , next_tick_(clock_.now() + tick_length_)
  , current_tick_bytes_(0)
  , n_low_ticks_(0)
  { }
  
  /*
   * Reports the time of the next tick, which is a good moment to check
   * for low throughput.
   */
  time_point_t next_tick() const
  {
    return next_tick_;
  }

  /*
   * Records a data transfer.  If the next tick is less than or equal
   * to the clock's current time, it is advanced to somewhere in the
   * future.
   */
  void record_transfer(std::size_t n_bytes)
  {
    this->update();

    if(n_bytes < min_bytes_per_tick_ - current_tick_bytes_)
    {
      current_tick_bytes_ += n_bytes;
    }
    else
    {
      n_low_ticks_ = 0;
      current_tick_bytes_ = min_bytes_per_tick_;
    }
  }

  /*
   * Tells if the throughput is low.  If the next tick is less than or
   * equal to the clock's current time, it is advanced to somewhere in
   * the future.
   */
  bool is_low()
  {
    this->update();

    return n_low_ticks_ >= low_ticks_limit_;
  }
  
private :
  void update()
  {
    time_point_t now = clock_.now();
    while(next_tick_ <= now)
    {
      if(current_tick_bytes_ < min_bytes_per_tick_ &&
         n_low_ticks_ < low_ticks_limit_)
      {
        ++n_low_ticks_;
      }

      current_tick_bytes_ = 0;
      next_tick_ += tick_length_;
    }
  }

private :
  ClockObjectT clock_;
  std::size_t min_bytes_per_tick_;
  unsigned int low_ticks_limit_;
  duration_t tick_length_;
  time_point_t next_tick_;
  std::size_t current_tick_bytes_;
  unsigned int n_low_ticks_;
};

} // cuti

#endif
