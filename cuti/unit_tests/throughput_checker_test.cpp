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

#include <cuti/throughput_checker.hpp>

#undef NDEBUG
#include <cassert>

namespace // anonymous
{

using namespace cuti;

void test_next_tick()
{
  /*
   * Verify that constructing a checker, recording a transfer and
   * checking for low speed each set the next tick to somewhere in the
   * future.
   */
  throughput_settings_t settings;
  settings.min_bytes_per_tick_ = 1;
  settings.low_ticks_limit_ = 1;
  settings.tick_length_ = seconds_t(1);
  
  time_point_t clock = cuti_clock_t::now();
  throughput_checker_t<user_clock_object_t> checker(
    settings, user_clock_object_t(clock));

  auto next = checker.next_tick();
  assert(next > clock);

  clock += seconds_t(1);
  checker.record_transfer(512);
  next = checker.next_tick();
  assert(next > clock);

  clock += seconds_t(1);
  checker.record_transfer(0);
  next = checker.next_tick();
  assert(next > clock);
}
  
void test_speed()
{
  {
    // a zero low ticks limit must report immediate and persistent failure
    throughput_settings_t settings;
    settings.min_bytes_per_tick_ = 512;
    settings.low_ticks_limit_ = 0;
    settings.tick_length_ = seconds_t(1);

    time_point_t clock = cuti_clock_t::now();
    throughput_checker_t<user_clock_object_t> checker(
      settings, user_clock_object_t(clock));
    assert(!checker.record_transfer(1024).ok());

    clock += seconds_t(1);
    assert(!checker.record_transfer(1024).ok());
  }

  {
    throughput_settings_t settings;
    settings.min_bytes_per_tick_ = 512;
    settings.low_ticks_limit_ = 1;
    settings.tick_length_ = seconds_t(1);

    time_point_t clock = cuti_clock_t::now();
    throughput_checker_t<user_clock_object_t> checker(
      settings, user_clock_object_t(clock));
    assert(checker.record_transfer(0).ok());

    clock += seconds_t(1);
    assert(!checker.record_transfer(0).ok());
  }

  {
    throughput_settings_t settings;
    settings.min_bytes_per_tick_ = 512;
    settings.low_ticks_limit_ = 2;
    settings.tick_length_ = seconds_t(1);

    time_point_t clock = cuti_clock_t::now();
    throughput_checker_t<user_clock_object_t> checker(
      settings, user_clock_object_t(clock));
    assert(checker.record_transfer(0).ok());

    clock += seconds_t(1);
    assert(checker.record_transfer(0).ok());

    clock += seconds_t(1);
    assert(!checker.record_transfer(0).ok());
  }

  {
    throughput_settings_t settings;
    settings.min_bytes_per_tick_ = 512;
    settings.low_ticks_limit_ = 1;
    settings.tick_length_ = seconds_t(1);

    time_point_t clock = cuti_clock_t::now();
    throughput_checker_t<user_clock_object_t> checker(
      settings, user_clock_object_t(clock));
    assert(checker.record_transfer(0).ok());

    clock += seconds_t(1);
    assert(!checker.record_transfer(0).ok());
  }

  {
    throughput_settings_t settings;
    settings.min_bytes_per_tick_ = 512;
    settings.low_ticks_limit_ = 2;
    settings.tick_length_ = seconds_t(1);

    time_point_t clock = cuti_clock_t::now();
    throughput_checker_t<user_clock_object_t> checker(
      settings, user_clock_object_t(clock));
    assert(checker.record_transfer(0).ok());

    clock += seconds_t(1);
    assert(checker.record_transfer(0).ok());

    clock += seconds_t(1);
    assert(!checker.record_transfer(0).ok());
  }

  {
    throughput_settings_t settings;
    settings.min_bytes_per_tick_ = 512;
    settings.low_ticks_limit_ = 1;
    settings.tick_length_ = seconds_t(1);

    time_point_t clock = cuti_clock_t::now();
    throughput_checker_t<user_clock_object_t> checker(
      settings, user_clock_object_t(clock));
    assert(checker.record_transfer(511).ok());

    clock += seconds_t(1);
    assert(!checker.record_transfer(0).ok());
  }

  {
    throughput_settings_t settings;
    settings.min_bytes_per_tick_ = 512;
    settings.low_ticks_limit_ = 2;
    settings.tick_length_ = seconds_t(1);

    time_point_t clock = cuti_clock_t::now();
    throughput_checker_t<user_clock_object_t> checker(
      settings, user_clock_object_t(clock));
    assert(checker.record_transfer(511).ok());

    clock += seconds_t(1);
    assert(checker.record_transfer(511).ok());

    clock += seconds_t(1);
    assert(!checker.record_transfer(0).ok());
  }

  {
    throughput_settings_t settings;
    settings.min_bytes_per_tick_ = 512;
    settings.low_ticks_limit_ = 1;
    settings.tick_length_ = seconds_t(1);

    time_point_t clock = cuti_clock_t::now();
    throughput_checker_t<user_clock_object_t> checker(
      settings, user_clock_object_t(clock));
    assert(checker.record_transfer(512).ok());

    clock += seconds_t(1);
    assert(checker.record_transfer(511).ok());

    clock += seconds_t(1);
    assert(!checker.record_transfer(0).ok());
  }

  {
    throughput_settings_t settings;
    settings.min_bytes_per_tick_ = 512;
    settings.low_ticks_limit_ = 120;
    settings.tick_length_ = seconds_t(1);

    time_point_t clock = cuti_clock_t::now();
    throughput_checker_t<user_clock_object_t> checker(
      settings, user_clock_object_t(clock));
    assert(checker.record_transfer(512).ok());

    for(unsigned int i = 0; i != 120; ++i)
    {
      assert(checker.record_transfer(511 - i).ok());
      clock += seconds_t(1);
      assert(checker.record_transfer(0).ok());
    }

    clock += seconds_t(1);
    assert(!checker.record_transfer(0).ok());
  }
}

} // anonymous

int main()
{
  test_next_tick();
  test_speed();

  return 0;
}
