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

#include <cuti/ref_args.hpp>

#include <utility>
#include <vector>

// enable assert()
#undef NDEBUG
#include <cassert>

namespace // anonymous
{

using namespace cuti;

struct three_vectors_t
{
  template<typename First, typename Second, typename Third>
  three_vectors_t(First&& first, Second&& second, Third&& third)
  : first_(std::forward<First>(first))
  , second_(std::forward<Second>(second))
  , third_(std::forward<Third>(third))
  { }

  bool equals(three_vectors_t const& other) const
  {
    return first_ == other.first_ &&
      second_ == other.second_ &&
      third_ == other.third_;
  }
  
private :
  std::vector<short> first_;
  std::vector<int> second_;
  std::vector<long> third_;
};

bool operator==(three_vectors_t const& lhs, three_vectors_t const& rhs)
{
  return lhs.equals(rhs);
}

template<typename T>
struct construct_t
{
  template<typename... Args>
  auto operator()(Args&&... args) const
  {
    return T{std::forward<Args>(args)...};
  }
};

template<typename T>
auto constexpr construct = construct_t<T>{};

std::vector<short> const short_vector{1};
std::vector<int> const int_vector{2, 3};
std::vector<long> const long_vector{4, 5, 6};

void test_construct()
{
  std::vector<short> sv = short_vector;
  std::vector<int> iv = int_vector;
  std::vector<long> lv = long_vector;

  three_vectors_t const expected(short_vector, int_vector, long_vector);

  auto constructed = construct<three_vectors_t>(sv, iv, lv);
  assert(constructed == expected);

  assert(sv == short_vector);
  assert(iv == int_vector);
  assert(lv == long_vector);

  constructed = construct<three_vectors_t>(
    std::move(sv), std::move(iv), std::move(lv));
  assert(constructed == expected);

  assert(sv.empty());
  assert(iv.empty());
  assert(lv.empty());
}

void test_apply_lvalues()
{
  std::vector<short> sv = short_vector;
  std::vector<int> iv = int_vector;
  std::vector<long> lv = long_vector;

  three_vectors_t const expected(short_vector, int_vector, long_vector);

  auto args = ref_args(sv, iv, lv);
  auto constructed = args.apply(construct<three_vectors_t>);

  assert(constructed == expected);

  assert(sv == short_vector);
  assert(iv == int_vector);
  assert(lv == long_vector);
}

void test_apply_rvalues()
{
  std::vector<short> sv = short_vector;
  std::vector<int> iv = int_vector;
  std::vector<long> lv = long_vector;

  three_vectors_t const expected(short_vector, int_vector, long_vector);

  auto args = ref_args(std::move(sv), std::move(iv), std::move(lv));
  auto constructed = args.apply(construct<three_vectors_t>);

  assert(constructed == expected);

  assert(sv.empty());
  assert(iv.empty());
  assert(lv.empty());
}

void test_apply_reversed_lvalues()
{
  std::vector<short> sv = short_vector;
  std::vector<int> iv = int_vector;
  std::vector<long> lv = long_vector;

  three_vectors_t const expected(short_vector, int_vector, long_vector);

  auto args = ref_args(lv, iv, sv);
  auto constructed = args.apply_reversed(construct<three_vectors_t>);

  assert(constructed == expected);

  assert(sv == short_vector);
  assert(iv == int_vector);
  assert(lv == long_vector);
}

void test_apply_reversed_rvalues()
{
  std::vector<short> sv = short_vector;
  std::vector<int> iv = int_vector;
  std::vector<long> lv = long_vector;

  three_vectors_t const expected(short_vector, int_vector, long_vector);

  auto args = ref_args(std::move(lv), std::move(iv), std::move(sv));
  auto constructed = args.apply_reversed(construct<three_vectors_t>);

  assert(constructed == expected);

  assert(sv.empty());
  assert(iv.empty());
  assert(lv.empty());
}

} // anonymous

int main()
{
  test_construct();
  test_apply_lvalues();
  test_apply_rvalues();
  test_apply_reversed_lvalues();
  test_apply_reversed_rvalues();

  return 0;
}
