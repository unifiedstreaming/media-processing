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

#include "list_arena.hpp"

#include <initializer_list>
#include <utility>

// Enable assert()
#undef NDEBUG
#include <cassert>

namespace // anoymous
{

using namespace cuti;

template<typename T>
void check_list(list_arena_t<T> const& arena, int list,
                std::initializer_list<T> const& expected)
{
  int element = arena.first(list);
  for(auto it = expected.begin(); it != expected.end(); ++it)
  {
    assert(element != arena.last(list));
    assert(arena.value(element) == *it);
    element = arena.next(element);
  }
  assert(element == arena.last(list));

  for(auto it = rbegin(expected); it != rend(expected); ++it)
  {
    assert(element != arena.first(list));
    element = arena.prev(element);
    assert(arena.value(element) == *it);
  }
  assert(element == arena.first(list));
}

void empty_list()
{
  list_arena_t<int> arena;

  int list = arena.add_list();
  check_list(arena, list, { });

  arena.remove_list(list);
}

void single_element()
{
  list_arena_t<int> arena;
  int list = arena.add_list();

  int element = arena.add_element(arena.last(list), 42);
  check_list(arena, list, { 42 });

  arena.move_element(element, element);
  check_list(arena, list, { 42 });
  
  arena.move_element(element, arena.last(list));
  check_list(arena, list, { 42 });

  arena.remove_element(element);
  check_list(arena, list, { });

  arena.remove_list(list);
}

void multiple_elements()
{
  list_arena_t<int> arena;
  int list = arena.add_list();

  int e4711 = arena.add_element(arena.last(list), 4711);
  int e42 = arena.add_element(e4711, 42);
  check_list(arena, list, { 42, 4711 });

  arena.move_element(e4711, e42);
  check_list(arena, list, { 4711, 42 } );

  arena.move_element(e4711, e4711);
  check_list(arena, list, { 4711, 42 } );

  arena.move_element(e4711, e42);
  check_list(arena, list, { 4711, 42 } );

  arena.remove_element(e4711);
  check_list(arena, list, { 42 } );
  
  arena.remove_element(e42);
  check_list(arena, list, { } );

  arena.remove_list(list);
}

void multiple_lists()
{
  list_arena_t<int> arena;
  int numbers = arena.add_list();
  int odds = arena.add_list();
  int evens = arena.add_list();

  for(int value = 1; value <= 6; ++value)
  {
    arena.add_element(arena.last(numbers), value);
  }
  check_list(arena, numbers, { 1, 2, 3, 4, 5, 6 });
  check_list(arena, odds, { });
  check_list(arena, evens, { });

  for(int element = arena.first(numbers);
      element != arena.last(numbers);
      element = arena.first(numbers))
  {
    if(arena.value(element) % 2 != 0)
    {
      arena.move_element(element, arena.last(odds));
    }
    else
    {
      arena.move_element(element, arena.last(evens));
    }
  }
  check_list(arena, numbers, { });
  check_list(arena, odds, { 1, 3, 5 });
  check_list(arena, evens, { 2, 4, 6 });

  while(arena.first(odds) != arena.last(odds) &&
        arena.first(evens) != arena.last(evens))
  {
    arena.move_element(arena.first(odds), arena.last(numbers));
    arena.move_element(arena.first(evens), arena.last(numbers));
  }
  while(arena.first(odds) != arena.last(odds))
  {
    arena.move_element(arena.first(odds), arena.last(numbers));
  }
  while(arena.first(evens) != arena.last(evens))
  {
    arena.move_element(arena.first(evens), arena.last(numbers));
  }
  check_list(arena, numbers, { 1, 2, 3, 4, 5, 6 });
  check_list(arena, odds, { });
  check_list(arena, evens, { });

  arena.remove_list(evens);
  check_list(arena, numbers, { 1, 2, 3, 4, 5, 6 });
  check_list(arena, odds, { });
  
  arena.remove_list(odds);
  check_list(arena, numbers, { 1, 2, 3, 4, 5, 6 });
  
  arena.remove_list(numbers);    
}

void list_reversal()
{
  list_arena_t<int> arena;
  int list = arena.add_list();
  for(int value = 1; value <= 6; ++value)
  {
    arena.add_element(arena.last(list), value);
  }
  check_list(arena, list, { 1, 2, 3, 4, 5, 6 });

  int pos = arena.first(list);
  if(pos != arena.last(list))
  {
    for(int next = arena.next(pos);
        next != arena.last(list);
	next = arena.next(pos))
    {
      arena.move_element(next, arena.first(list));
    }
  }
  check_list(arena, list, { 6, 5, 4, 3, 2, 1 });

  arena.remove_list(list);
}

struct counted_t
{
  static int count;

  counted_t()
  { ++count; }

  counted_t(counted_t const&)
  { ++count; }

  ~counted_t()
  { --count; }
};

int counted_t::count = 0;

void ctors_and_dtors()
{
  list_arena_t<counted_t> arena1;
  int list = arena1.add_list();

  for(int i = 0; i != 6; ++i)
  {
    arena1.add_element(arena1.last(list));
  }
  assert(counted_t::count == 6);

  list_arena_t<counted_t> arena2 = arena1;
  assert(counted_t::count == 12);

  arena1 = list_arena_t<counted_t>();
  assert(counted_t::count == 6);

  arena1 = std::move(arena2);
  assert(counted_t::count == 6);

  arena1.remove_list(list);
  assert(counted_t::count == 0);
}

} // anonymous

int main()
{
  empty_list();
  single_element();
  multiple_elements();
  multiple_lists();
  list_reversal();
  ctors_and_dtors();

  return 0;
}
