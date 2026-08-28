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

#include <cuti/mutex_wrapper.hpp>

#include <cuti/chrono_types.hpp>
#include <cuti/scoped_thread.hpp>

#include <atomic>
#include <memory>
#include <thread>
#include <type_traits>
#include <vector>

// enable assert()
#undef NDEBUG
#include <cassert>

namespace // anonymous
{

using namespace cuti;

struct base_t { };
struct derived_t : base_t { };

struct point_t
{
  point_t(int x, int y)
  : x_(x)
  , y_(y)
  { }

  int x_;
  int y_;
};

struct unprotected_t
{
  unprotected_t()
  : consistent_(true)
  { }

  void modify()
  {
    assert(consistent_.load() == true);
    consistent_.store(false);
    assert(consistent_.load() == false);

    std::this_thread::sleep_for(milliseconds_t{100});
    
    assert(consistent_.load() == false);
    consistent_.store(true);
    assert(consistent_.load() == true);
  }

private :
  std::atomic<bool> consistent_;
};

void static_checks()
{
  using wrapper_t = mutex_wrapper_t<int>;
  using lock_t = wrapper_t::lock_t;

  static_assert(!std::is_copy_constructible_v<wrapper_t>);
  static_assert(!std::is_move_constructible_v<wrapper_t>);

  static_assert(!std::is_copy_assignable_v<wrapper_t>);
  static_assert(!std::is_move_assignable_v<wrapper_t>);

  static_assert(std::is_nothrow_default_constructible_v<lock_t>);

  static_assert(!std::is_copy_constructible_v<lock_t>);
  static_assert(std::is_nothrow_move_constructible_v<lock_t>);

  static_assert(!std::is_copy_assignable_v<lock_t>);
  static_assert(std::is_nothrow_move_assignable_v<lock_t>);
}

void null_locks()
{
  {
    mutex_wrapper_t<int>::lock_t lock{};

    assert(!lock);

    assert(lock == nullptr);
    assert(nullptr == lock);

    assert(!(lock != nullptr));
    assert(!(nullptr != lock));
  }
  
  {
    mutex_wrapper_t<int>::lock_t lock{nullptr};

    assert(!lock);

    assert(lock == nullptr);
    assert(nullptr == lock);

    assert(!(lock != nullptr));
    assert(!(nullptr != lock));
  }
  
  {
    mutex_wrapper_t<int>::const_lock_t lock{};

    assert(!lock);

    assert(lock == nullptr);
    assert(nullptr == lock);

    assert(!(lock != nullptr));
    assert(!(nullptr != lock));
  }
  
  {
    mutex_wrapper_t<int>::const_lock_t lock{nullptr};
    assert(!lock);

    assert(lock == nullptr);
    assert(nullptr == lock);

    assert(!(lock != nullptr));
    assert(!(nullptr != lock));
  }
}

void non_null_locks()
{
  {
    mutex_wrapper_t<int> wrapper{};
    auto lock = wrapper.lock();

    assert(lock);

    assert(!(lock == nullptr));
    assert(!(nullptr == lock));

    assert(lock != nullptr);
    assert(nullptr != lock);
  }

  {
    mutex_wrapper_t<int> const wrapper{};
    auto lock = wrapper.lock();

    assert(lock);

    assert(!(lock == nullptr));
    assert(!(nullptr == lock));

    assert(lock != nullptr);
    assert(nullptr != lock);
  }
}

void moving_locks()
{
  {
    mutex_wrapper_t<int>::lock_t lock1{};
    assert(lock1 == nullptr);

    mutex_wrapper_t<int>::lock_t lock2{std::move(lock1)};
    assert(lock1 == nullptr);
    assert(lock2 == nullptr);
  }
  
  {
    mutex_wrapper_t<int> wrapper{};

    mutex_wrapper_t<int>::lock_t lock1{wrapper.lock()};
    assert(lock1 != nullptr);

    mutex_wrapper_t<int>::lock_t lock2{std::move(lock1)};
    assert(lock1 == nullptr);
    assert(lock2 != nullptr);
  }

  {
    mutex_wrapper_t<int>::lock_t lock1{};
    assert(lock1 == nullptr);

    mutex_wrapper_t<int>::lock_t lock2{};
    lock2 = std::move(lock1);
    assert(lock1 == nullptr);
    assert(lock2 == nullptr);
  }
  
  {
    mutex_wrapper_t<int> wrapper{};

    mutex_wrapper_t<int>::lock_t lock1{wrapper.lock()};
    assert(lock1 != nullptr);

    mutex_wrapper_t<int>::lock_t lock2{};
    lock2 = std::move(lock1);
    assert(lock1 == nullptr);
    assert(lock2 != nullptr);
  }
}  
  
void lock_conversions()
{
  {
    mutex_wrapper_t<int>::lock_t lock1{};
    assert(lock1 == nullptr);

    mutex_wrapper_t<int>::const_lock_t lock2{std::move(lock1)};
    assert(lock1 == nullptr);
    assert(lock2 == nullptr);
  }
  
  {
    mutex_wrapper_t<int> wrapper{};

    mutex_wrapper_t<int>::lock_t lock1{wrapper.lock()};
    assert(lock1 != nullptr);

    mutex_wrapper_t<int>::const_lock_t lock2{std::move(lock1)};
    assert(lock1 == nullptr);
    assert(lock2 != nullptr);
  }

  {
    mutex_wrapper_t<int>::lock_t lock1{};
    assert(lock1 == nullptr);

    mutex_wrapper_t<int>::const_lock_t lock2{};
    lock2 = std::move(lock1);
    assert(lock1 == nullptr);
    assert(lock2 == nullptr);
  }
  
  {
    mutex_wrapper_t<int> wrapper{};

    mutex_wrapper_t<int>::lock_t lock1{wrapper.lock()};
    assert(lock1 != nullptr);

    mutex_wrapper_t<int>::const_lock_t lock2{};
    lock2 = std::move(lock1);
    assert(lock1 == nullptr);
    assert(lock2 != nullptr);
  }

  {
    mutex_wrapper_t<derived_t>::lock_t lock1{};
    assert(lock1 == nullptr);

    mutex_wrapper_t<base_t>::lock_t lock2{std::move(lock1)};
    assert(lock1 == nullptr);
    assert(lock2 == nullptr);
  }
  
  {
    mutex_wrapper_t<derived_t> wrapper{};

    mutex_wrapper_t<derived_t>::lock_t lock1{wrapper.lock()};
    assert(lock1 != nullptr);

    mutex_wrapper_t<base_t>::lock_t lock2{std::move(lock1)};
    assert(lock1 == nullptr);
    assert(lock2 != nullptr);
  }

  {
    mutex_wrapper_t<derived_t>::lock_t lock1{};
    assert(lock1 == nullptr);

    mutex_wrapper_t<base_t>::lock_t lock2{};
    lock2 = std::move(lock1);
    assert(lock1 == nullptr);
    assert(lock2 == nullptr);
  }
  
  {
    mutex_wrapper_t<derived_t> wrapper{};

    mutex_wrapper_t<derived_t>::lock_t lock1{wrapper.lock()};
    assert(lock1 != nullptr);

    mutex_wrapper_t<base_t>::lock_t lock2{};
    lock2 = std::move(lock1);
    assert(lock1 == nullptr);
    assert(lock2 != nullptr);
  }
}

void read_access()
{
  {
    mutex_wrapper_t<int> wrapper{42};
    auto lock = wrapper.lock();
    assert(*lock == 42);
  }

  {
    mutex_wrapper_t<point_t> wrapper{43, 44};
    auto lock = wrapper.lock();
    assert(lock->x_ == 43);
    assert(lock->y_ == 44);
  }

  {
    mutex_wrapper_t<int const> wrapper{42};
    auto lock = wrapper.lock();
    assert(*lock == 42);
  }

  {
    mutex_wrapper_t<point_t const> wrapper{43, 44};
    auto lock = wrapper.lock();
    assert(lock->x_ == 43);
    assert(lock->y_ == 44);
  }

  {
    mutex_wrapper_t<int> const wrapper{42};
    auto lock = wrapper.lock();
    assert(*lock == 42);
  }

  {
    mutex_wrapper_t<point_t> const wrapper{43, 44};
    auto lock = wrapper.lock();
    assert(lock->x_ == 43);
    assert(lock->y_ == 44);
  }
}

void write_access()
{
  {
    mutex_wrapper_t<int> wrapper{42};

    {
      auto lock = wrapper.lock();
      *lock = 43;
      assert(*lock == 43);
    }

    {
      auto lock = wrapper.lock();
      assert(*lock == 43);
    }
  }

  {
    mutex_wrapper_t<point_t> wrapper{44, 45};

    {
      auto lock = wrapper.lock();
      lock->x_ = 46;
      lock->y_ = 47;
      assert(lock->x_ == 46);
      assert(lock->y_ == 47);
    }

    {
      auto lock = wrapper.lock();
      assert(lock->x_ == 46);
      assert(lock->y_ == 47);
    }
  }
}

void concurrent_access()
{
  mutex_wrapper_t<unprotected_t> wrapper{};
  auto thread_body = [&wrapper]()
  {
    auto target = wrapper.lock();
    target->modify();
  };

  {
    std::vector<std::unique_ptr<scoped_thread_t>> threads;
    while(threads.size() != 10)
    {
      threads.push_back(std::make_unique<scoped_thread_t>(thread_body));
    }
  }
}

void unlock()
{
  mutex_wrapper_t<int> wrapper{42};
  auto main_lock = wrapper.lock();

  scoped_thread_t thread{[&wrapper]()
  {
    auto thread_lock = wrapper.lock();
    assert(*thread_lock == 43);
  }};

  *main_lock = 43;
  main_lock.unlock();
  assert(main_lock == nullptr);
}
  
} // anonymous

int main()
{
  static_checks();
  null_locks();
  non_null_locks();
  moving_locks();
  lock_conversions();
  read_access();
  write_access();
  concurrent_access();
  unlock();

  return 0;
}
