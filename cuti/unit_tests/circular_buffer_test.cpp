/*
 * Copyright (C) 2021-2023 CodeShop B.V.
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

#include <cuti/circular_buffer.hpp>

#include <algorithm>
#include <exception>
#include <iostream>
#include <utility>

// Enable assert()
#undef NDEBUG
#include <cassert>

namespace // anonymous
{

using namespace cuti;

void validate_buffer(circular_buffer_t&& buffer, char const* expected)
{
  assert(buffer.capacity() ==
    buffer.total_data_size() + buffer.total_slack_size());

  if(*expected == '\0')
  {
    buffer.push_back(buffer.begin_slack());
    buffer.pop_front(buffer.begin_data());
  }
  else for(; *expected != '\0'; ++expected)
  {
    assert(buffer.total_data_size() != 0);
    assert(buffer.has_data());
    assert(buffer.begin_data() < buffer.end_data());
    assert(*buffer.begin_data() == *expected);

    buffer.pop_front(buffer.begin_data() + 1);

    assert(buffer.total_slack_size() != 0);
    assert(buffer.has_slack());
    assert(buffer.begin_slack() < buffer.end_slack());
  }

  assert(buffer.total_data_size() == 0);
  assert(!buffer.has_data());
  assert(buffer.begin_data() == buffer.end_data());

  assert(buffer.total_slack_size() == buffer.capacity());

  if(!buffer.has_slack())
  {
    assert(buffer.total_slack_size() == 0);
    assert(buffer.begin_slack() == buffer.end_slack());
  }
  else
  {
    assert(buffer.total_slack_size() != 0);
    assert(buffer.begin_slack() < buffer.end_slack());
  }
}

void check_buffer(circular_buffer_t&& buffer, char const* expected)
{
  circular_buffer_t copy_constructed(buffer);
  circular_buffer_t copy_assigned;
  copy_assigned = buffer;

  circular_buffer_t moved_from_1(buffer);

  circular_buffer_t move_constructed = std::move(moved_from_1);
  assert(!moved_from_1.has_slack());
  assert(moved_from_1.begin_slack() == moved_from_1.end_slack());
  assert(!moved_from_1.has_data());
  assert(moved_from_1.begin_data() == moved_from_1.end_data());
  
  circular_buffer_t moved_from_2(buffer);

  circular_buffer_t move_assigned;
  move_assigned = std::move(moved_from_2);
  assert(!moved_from_2.has_slack());
  assert(moved_from_2.begin_slack() == moved_from_2.end_slack());
  assert(!moved_from_2.has_data());
  assert(moved_from_2.begin_data() == moved_from_2.end_data());

  validate_buffer(std::move(buffer), expected);
  validate_buffer(std::move(copy_constructed), expected);
  validate_buffer(std::move(move_constructed), expected);
  validate_buffer(std::move(move_assigned), expected);
}

void default_buffer()
{
  circular_buffer_t buffer;
  assert(!buffer.has_slack());

  check_buffer(std::move(buffer), "");
}
  
void zero_capacity_buffer()
{
  circular_buffer_t buffer(0);
  assert(!buffer.has_slack());

  check_buffer(std::move(buffer), "");
}

void small_empty()
{
  circular_buffer_t buffer(1);
  assert(buffer.has_slack());
  
  check_buffer(std::move(buffer), "");
}

void large_empty()
{
  circular_buffer_t buffer(3);
  assert(buffer.has_slack());

  check_buffer(std::move(buffer), "");
}

void small_full()
{
  circular_buffer_t buffer(1);

  *buffer.begin_slack() = '1';
  buffer.push_back(buffer.begin_slack() + 1);
  assert(!buffer.has_slack());
  
  check_buffer(std::move(buffer), "1");
}

void large_full()
{
  circular_buffer_t buffer(3);

  char const data[] = { '1', '2', '3' };
  std::copy(data, data + 3, buffer.begin_slack());
  buffer.push_back(buffer.begin_slack() + 3);
  assert(!buffer.has_slack());

  check_buffer(std::move(buffer), "123");
}

void half_full()
{
  circular_buffer_t buffer(2);

  *buffer.begin_slack() = '1';
  buffer.push_back(buffer.begin_slack() + 1);
  assert(buffer.has_slack());

  check_buffer(std::move(buffer), "1");
}

void wrapped_slack()
{
  circular_buffer_t buffer(3);

  char const data[] = { '1', '2' };
  std::copy(data, data + 2, buffer.begin_slack());
  buffer.push_back(buffer.begin_slack() + 2);
  assert(buffer.has_slack());
  assert(buffer.has_data());

  buffer.pop_front(buffer.begin_data() + 1);
  assert(buffer.has_slack());

  check_buffer(std::move(buffer), "2");
}
  
void wrapped_data()
{
  circular_buffer_t buffer(3);

  char const data[] = { '1', '2', '3' };
  std::copy(data, data + 3, buffer.begin_slack());
  buffer.push_back(buffer.begin_slack() + 3);
  assert(!buffer.has_slack());
  assert(buffer.has_data());

  buffer.pop_front(buffer.begin_data() + 2);
  assert(buffer.has_slack());
  assert(buffer.has_data());

  *buffer.begin_slack() = '4';
  buffer.push_back(buffer.begin_slack() + 1);
  assert(buffer.has_slack());

  check_buffer(std::move(buffer), "34");
}

void reserve_too_small()
{
  circular_buffer_t buffer(1);
  assert(buffer.capacity() == 1);

  *buffer.begin_slack() = '1';
  buffer.push_back(buffer.begin_slack() + 1);

  buffer.reserve(0);
  assert(buffer.capacity() == 1);

  check_buffer(std::move(buffer), "1");
}  
  
void reserve_to_capacity()
{
  circular_buffer_t buffer(1);
  assert(buffer.capacity() == 1);

  *buffer.begin_slack() = '1';
  buffer.push_back(buffer.begin_slack() + 1);

  buffer.reserve(buffer.capacity());
  assert(buffer.capacity() == 1);

  check_buffer(std::move(buffer), "1");
}  
  
void shrink_to_fit()
{
  circular_buffer_t buffer(2);
  assert(buffer.capacity() == 2);

  *buffer.begin_slack() = '1';
  buffer.push_back(buffer.begin_slack() + 1);

  buffer.reserve(buffer.total_data_size());
  assert(buffer.capacity() == 1);

  check_buffer(std::move(buffer), "1");
}

void enlarge_slack()
{
  circular_buffer_t buffer(1);
  assert(buffer.capacity() == 1);

  *buffer.begin_slack() = '1';
  buffer.push_back(buffer.begin_slack() + 1);

  buffer.reserve(buffer.total_data_size() + 1);
  assert(buffer.capacity() == 2);

  check_buffer(std::move(buffer), "1");
}

void run_tests(int, char const* const*)
{
  default_buffer();
  zero_capacity_buffer();
  small_empty();
  large_empty();
  small_full();
  large_full();
  half_full();
  wrapped_slack();
  wrapped_data();

  reserve_too_small();
  reserve_to_capacity();
  shrink_to_fit();
  enlarge_slack();
}

} // anonymous

int main(int argc, char* argv[])
{
  try
  {
    run_tests(argc, argv);
  }
  catch(std::exception const& ex)
  {
    std::cerr << argv[0] << ": exception: " << ex.what() << std::endl;
    throw;
  }

  return 0;
}
