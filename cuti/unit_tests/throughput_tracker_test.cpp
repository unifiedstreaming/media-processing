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

#include <cuti/throughput_tracker.hpp>

#include <thread>

#undef NDEBUG
#include <cassert>

namespace // anonymous
{

using namespace cuti;

void await(time_point_t until)
{
  auto now = cuti_clock_t::now();
  while(now < until)
  {
    std::this_thread::sleep_for(until - now);
    now = cuti_clock_t::now();
  }
}
    
void test_next_tick()
{
  /*
   * Verify that constructing a tracker, recording a transfer and
   * checking for low speed each set the next tick to somewhere in the
   * future.
   */
  auto now = cuti_clock_t::now();
  throughput_tracker_t tracker(1, 1, milliseconds_t(1));
  auto next = tracker.next_tick();
  assert(next > now);

  await(next);
  now = next;

  tracker.record_transfer(0);
  next = tracker.next_tick();
  assert(next > now);

  await(next);
  now = next;

  tracker.is_low();
  next = tracker.next_tick();
  assert(next > now);
}
  
void test_low_speed()
{
  {
    throughput_tracker_t tracker(512, 1, milliseconds_t(1));

    await(tracker.next_tick());
    assert(tracker.is_low());
  }

  {
    throughput_tracker_t tracker(512, 2, milliseconds_t(1));

    await(tracker.next_tick());
    tracker.is_low(); // unlikely, but advances next_tick

    await(tracker.next_tick());
    assert(tracker.is_low()); // must be true after second tick
  }

  {
    throughput_tracker_t tracker(512, 1, milliseconds_t(1));
    tracker.record_transfer(0);

    await(tracker.next_tick());
    assert(tracker.is_low());
  }

  {
    throughput_tracker_t tracker(512, 2, milliseconds_t(1));
    tracker.record_transfer(0);

    await(tracker.next_tick());
    tracker.record_transfer(0); // advances next_tick

    await(tracker.next_tick());
    assert(tracker.is_low()); // must be true after second tick
  }

  {
    throughput_tracker_t tracker(512, 1, milliseconds_t(1));
    tracker.record_transfer(511);

    await(tracker.next_tick());
    assert(tracker.is_low());
  }

  {
    throughput_tracker_t tracker(512, 2, milliseconds_t(1));
    tracker.record_transfer(511);

    await(tracker.next_tick());
    tracker.record_transfer(511); // advances next_tick

    await(tracker.next_tick());
    assert(tracker.is_low()); // must be true after second tick
  }

  {
    throughput_tracker_t tracker(512, 1, milliseconds_t(1));
    tracker.record_transfer(512);

    await(tracker.next_tick());
    tracker.record_transfer(511); // advances next_tick

    await(tracker.next_tick());
    assert(tracker.is_low()); // must be true after second tick
  }
}

void test_sufficient_speed()
{
  throughput_tracker_t tracker(512, 100000, milliseconds_t(1));
  tracker.record_transfer(512);

  assert(!tracker.is_low()); // only low if 100 seconds have passed
}

} // anonymous

int main()
{
  test_next_tick();
  test_low_speed();
  test_sufficient_speed();

  return 0;
}
