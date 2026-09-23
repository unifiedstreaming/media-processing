/*
 * Copyright (C) 2026 CodeShop B.V.
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

#include <cuti/serial_generator.hpp>

#include <cuti/mutex_wrapper.hpp>
#include <cuti/scoped_thread.hpp>

#include <algorithm>
#include <memory>
#include <thread>
#include <vector>

// enable assert
#undef NDEBUG
#include <cassert>

namespace // anonymous
{

using namespace cuti;

void single_threaded_test()
{
  serial_generator_t generator;

  for(unsigned int expected = 0; expected != 10; ++expected)
  {
    assert(generator.next() == expected);
  }
}

void multi_threaded_test()
{
  serial_generator_t generator;
  mutex_wrapper_t<std::vector<unsigned int>> serials;

  {
    auto thread_body = [&generator, &serials]
    {
      std::this_thread::yield();
      auto serial = generator.next();
      auto serials_lock = serials.lock();
      serials_lock->push_back(serial);
    };

    std::vector<std::unique_ptr<scoped_thread_t>> threads;
    for(unsigned int i = 0; i != 10; ++i)
    {
      threads.push_back(std::make_unique<scoped_thread_t>(thread_body));
    }
  }

  auto serials_lock = serials.lock();
  assert(serials_lock->size() == 10);
  for(unsigned int expected = 0; expected != 10; ++expected)
  {
    auto pos = std::find(
      serials_lock->begin(), serials_lock->end(), expected);
    assert(pos != serials_lock->end());
  }
}
  
void run_tests()
{
  single_threaded_test();
  multi_threaded_test();
}

} // anonymous

int main()
{
  run_tests();
  return 0;
}
