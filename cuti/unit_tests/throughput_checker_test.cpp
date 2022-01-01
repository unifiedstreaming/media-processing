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
  time_point_t clock = cuti_clock_t::now();
  throughput_checker_t<user_clock_object_t> checker(
    1, 1, seconds_t(1), user_clock_object_t(clock));

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
    time_point_t clock = cuti_clock_t::now();
    throughput_checker_t<user_clock_object_t> checker(
      512, 1, seconds_t(1), user_clock_object_t(clock));
    assert(checker.record_transfer(0) == 0);

    clock += seconds_t(1);
    assert(checker.record_transfer(0) != 0);
  }

  {
    time_point_t clock = cuti_clock_t::now();
    throughput_checker_t<user_clock_object_t> checker(
      512, 2, seconds_t(1), user_clock_object_t(clock));
    assert(checker.record_transfer(0) == 0);

    clock += seconds_t(1);
    assert(checker.record_transfer(0) == 0);

    clock += seconds_t(1);
    assert(checker.record_transfer(0) != 0);
  }

  {
    time_point_t clock = cuti_clock_t::now();
    throughput_checker_t<user_clock_object_t> checker(
      512, 1, seconds_t(1), user_clock_object_t(clock));
    assert(checker.record_transfer(0) == 0);

    clock += seconds_t(1);
    assert(checker.record_transfer(0) != 0);
  }

  {
    time_point_t clock = cuti_clock_t::now();
    throughput_checker_t<user_clock_object_t> checker(
      512, 2, seconds_t(1), user_clock_object_t(clock));
    assert(checker.record_transfer(0) == 0);

    clock += seconds_t(1);
    assert(checker.record_transfer(0) == 0);

    clock += seconds_t(1);
    assert(checker.record_transfer(0) != 0);
  }

  {
    time_point_t clock = cuti_clock_t::now();
    throughput_checker_t<user_clock_object_t> checker(
      512, 1, seconds_t(1), user_clock_object_t(clock));
    assert(checker.record_transfer(511) == 0);

    clock += seconds_t(1);
    assert(checker.record_transfer(0) != 0);
  }

  {
    time_point_t clock = cuti_clock_t::now();
    throughput_checker_t<user_clock_object_t> checker(
      512, 2, seconds_t(1), user_clock_object_t(clock));
    assert(checker.record_transfer(511) == 0);

    clock += seconds_t(1);
    assert(checker.record_transfer(511) == 0);

    clock += seconds_t(1);
    assert(checker.record_transfer(0) != 0);
  }

  {
    time_point_t clock = cuti_clock_t::now();
    throughput_checker_t<user_clock_object_t> checker(
      512, 1, seconds_t(1), user_clock_object_t(clock));
    assert(checker.record_transfer(512) == 0);

    clock += seconds_t(1);
    assert(checker.record_transfer(511) == 0);

    clock += seconds_t(1);
    assert(checker.record_transfer(0) != 0);
  }

  {
    time_point_t clock = cuti_clock_t::now();
    throughput_checker_t<user_clock_object_t> checker(
      512, 120, seconds_t(1), user_clock_object_t(clock));
    assert(checker.record_transfer(512) == 0);

    for(unsigned int i = 0; i != 120; ++i)
    {
      assert(checker.record_transfer(511 - i) == 0);
      clock += seconds_t(1);
      assert(checker.record_transfer(0) == 0);
    }

    clock += seconds_t(1);
    assert(checker.record_transfer(0) != 0);
  }
}

} // anonymous

int main()
{
  test_next_tick();
  test_speed();

  return 0;
}
