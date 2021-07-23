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

#include <cuti/oneshot.hpp>

#include <functional>
#include <optional>
#include <utility>

// Enable assert()
#undef NDEBUG
#include <cassert>

namespace /* anonymous */
{

using namespace cuti;

struct copy_move_counter_t
{
  copy_move_counter_t()
  : n_copies_(0)
  , n_moves_(0)
  { }

  copy_move_counter_t(copy_move_counter_t const& rhs)
  : n_copies_(rhs.n_copies_ + 1)
  , n_moves_(rhs.n_moves_)
  { }

  copy_move_counter_t(copy_move_counter_t&& rhs)
  : n_copies_(rhs.n_copies_)
  , n_moves_(rhs.n_moves_ + 1)
  { }

  int n_copies() const
  {
    return n_copies_;
  }

  int n_moves() const
  {
    return n_moves_;
  }

private :
  int const n_copies_;
  int const n_moves_;
};

copy_move_counter_t make_counter()
{
  return copy_move_counter_t();
}

struct counter_result_t
{
  counter_result_t()
  : opt_counter_(std::nullopt)
  { }

  counter_result_t(counter_result_t&) = delete;
  counter_result_t& operator=(counter_result_t&) = delete;
  
  void set(copy_move_counter_t const& counter)
  {
    assert(!opt_counter_.has_value());
    opt_counter_.emplace(counter);
  }

  void set(copy_move_counter_t&& counter)
  {
    assert(!opt_counter_.has_value());
    opt_counter_.emplace(std::move(counter));
  }

  copy_move_counter_t const& value() const
  {
    assert(opt_counter_.has_value());
    return *opt_counter_;
  }

private :
  std::optional<copy_move_counter_t> opt_counter_;
};
  
void restarted_value(counter_result_t& result, int count, int max,
                     copy_move_counter_t counter)
{
  if(count < max)
  {
    auto oneshot = make_oneshot(restarted_value, std::ref(result),
      count + 1, max, std::move(counter));
    oneshot();
    return;
  }

  result.set(std::move(counter));
}
    
void restarted_copy(counter_result_t& result, int count, int max,
                    copy_move_counter_t const& counter)
{
  if(count < max)
  {
    auto oneshot = make_oneshot(restarted_copy, std::ref(result),
      count + 1, max, counter);
    oneshot();
    return;
  }

  result.set(counter);
}
    
void restarted_move(counter_result_t& result, int count, int max,
                    copy_move_counter_t&& counter)
{
  if(count < max)
  {
    auto oneshot = make_oneshot(restarted_move, std::ref(result),
      count + 1, max, std::move(counter));
    oneshot();
    return;
  }

  result.set(std::move(counter));
}
    
void test_copy_move_counter()
{
  auto cnt1 = make_counter(); // copy elision expected
  assert(cnt1.n_copies() == 0);
  assert(cnt1.n_moves() == 0);

  auto cnt2 = cnt1;
  assert(cnt2.n_copies() == 1);
  assert(cnt2.n_moves() == 0);
  
  auto cnt3 = std::move(cnt1);
  assert(cnt3.n_copies() == 0);
  assert(cnt3.n_moves() == 1);
}

void test_counter_result()
{
  counter_result_t r1;
  r1.set(make_counter());
  assert(r1.value().n_copies() == 0);
  assert(r1.value().n_moves() == 1);

  counter_result_t r2;
  r2.set(r1.value());
  assert(r2.value().n_copies() == 1);
  assert(r2.value().n_moves() == 1);
}

void test_restarted_value()
{
  counter_result_t r;
  restarted_value(r, 0, 10, make_counter());
  assert(r.value().n_copies() == 0);
  assert(r.value().n_moves() == 21);
}
  
void test_restarted_copy()
{
  counter_result_t r;
  restarted_copy(r, 0, 10, make_counter());
  assert(r.value().n_copies() == 11);
  assert(r.value().n_moves() == 0);
}
  
void test_restarted_move()
{
  counter_result_t r;
  restarted_move(r, 0, 10, make_counter());
  assert(r.value().n_copies() == 0);
  assert(r.value().n_moves() == 11);
}
  
} // anonymous

int main()
{
  test_copy_move_counter();
  test_counter_result();
  test_restarted_value();
  test_restarted_move();
  test_restarted_copy();

  return 0;
}
