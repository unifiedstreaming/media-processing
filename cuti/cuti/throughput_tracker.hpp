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

#include "chrono_types.hpp"
#include "linkage.h"

#include <cstddef>

namespace cuti
{

struct CUTI_ABI throughput_tracker_t
{
  /*
   * Constructs a throughput tracker.  The throughput is considered to
   * be low if less than min_bytes_per_tick were transferred for at
   * least low_ticks_limit ticks.
   */
  throughput_tracker_t(std::size_t min_bytes_per_tick,
                       unsigned int low_ticks_limit,
                       duration_t tick_length = seconds_t(1));

  /*
   * Reports the time of the next tick, which is a good moment to check
   * for low throughput.
   */
  time_point_t next_tick() const
  {
    return next_tick_;
  }

  /*
   * Records a data transfer.  If the next tick is less than or equal to
   * the current time, it is advanced to somewhere in the future.
   */
  void record_transfer(std::size_t n_bytes);

  /*
   * Tells if the throughput is low.  If the next tick is less than or
   * equal to the current time, it is advanced to somewhere in the future.
   */
  bool is_low();

private :
  void update();

private :
  std::size_t min_bytes_per_tick_;
  unsigned int low_ticks_limit_;
  duration_t tick_length_;
  time_point_t next_tick_;
  std::size_t current_tick_bytes_;
  unsigned int n_low_ticks_;
};

} // cuti

#endif
