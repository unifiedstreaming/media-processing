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

#include "indexed_heap.hpp"

#include <vector>

// Enable assert
#undef NDEBUG
#include <cassert>

namespace // anonymous
{

using namespace cuti;

void maxheap()
{
  indexed_heap_t<int, int> q;
  assert(q.empty());

  std::vector<int> ids;
  for(int prio = 0; prio != 256; ++prio)
  {
    int id = q.add_element(prio, prio + 42);
    ids.push_back(id);
    
    assert(!q.empty());
    assert(q.front_element() == id);
    assert(q.priority(id) == prio);
    assert(q.value(id) == prio + 42);
  }

  int prio  = 256;
  while(!ids.empty())
  {
    int id = ids.back(); 
    --prio;
 
    assert(!q.empty());
    assert(q.front_element() == id);
    assert(q.priority(id) == prio);
    assert(q.value(id) == prio + 42);

    q.remove_element(id);
    ids.pop_back();
  }

  assert(q.empty());
}

void minheap()
{
  indexed_heap_t<int, int, std::greater<int>> q;
  assert(q.empty());

  std::vector<int> ids;
  for(int prio = 0; prio != 256; ++prio)
  {
    int id = q.add_element(prio, prio + 42);
    ids.push_back(id);
    
    assert(!q.empty());
    assert(q.front_element() == ids.front());
    assert(q.priority(id) == prio);
    assert(q.value(id) == prio + 42);
  }

  int prio = 0;
  for(int id : ids)
  {
    assert(!q.empty());
    assert(q.front_element() == id);
    assert(q.priority(id) == prio);
    assert(q.value(id) == prio + 42);

    q.remove_element(id);
    ++prio;
  }

  assert(q.empty());
}

void duplicate_prios_maxheap()
{
  indexed_heap_t<int, int> q;
  assert(q.empty());

  for(int value = 0; value != 256; ++value)
  {
    int id = q.add_element(value % 16, value);
    assert(!q.empty());
    assert(q.priority(id) == value % 16);
    assert(q.value(id) == value);
  }

  int prio = 16;
  while(prio != 0)
  {
    --prio;
    for(int count = 0; count != 16; ++count)
    {
      assert(!q.empty());
      int id = q.front_element();
      assert(q.priority(id) == prio);
      assert(q.value(id) % 16 == prio);
      q.remove_element(id);
    }
  }

  assert(q.empty());
}  
    
void duplicate_prios_minheap()
{
  indexed_heap_t<int, int, std::greater<int>> q;
  assert(q.empty());

  for(int value = 0; value != 256; ++value)
  {
    int id = q.add_element(value % 16, value);
    assert(!q.empty());
    assert(q.priority(id) == value % 16);
    assert(q.value(id) == value);
  }

  for(int prio = 0; prio != 16; ++prio)
  {
    for(int count = 0; count != 16; ++count)
    {
      assert(!q.empty());
      int id = q.front_element();
      assert(q.priority(id) == prio);
      assert(q.value(id) % 16 == prio);
      q.remove_element(id);
    }
  }

  assert(q.empty());
}  
    
void remove_non_front_ids()
{
  indexed_heap_t<int, int, std::greater<int>> q; // minheap
  assert(q.empty());

  std::vector<int> ids;
  for(int value = 0; value != 256; ++value)
  {
    int id = q.add_element(value % 16, value);
    assert(!q.empty());
    assert(q.priority(id) == value % 16);
    assert(q.value(id) == value);

    ids.push_back(id);
  }

  // remove half of the elements from the middle of ids
  while(ids.size() > 128)
  {
    auto index = ids.size() / 2;
    int id = ids[index];

    assert(!q.empty());
    assert(q.priority(id) == q.value(id) % 16);
    q.remove_element(id);
    
    ids[index] = ids.back();
    ids.pop_back();
  }

  // check priority order of remaining elements
  int prio = 0;
  for(int i = 0; i != 128; ++i)
  {
    assert(!q.empty());
    int id = q.front_element();

    assert(q.priority(id) >= prio);
    prio = q.priority(id);
    assert(prio == q.value(id) % 16);

    q.remove_element(id);
  }
    
  assert(q.empty());
}  
    
template<typename Q>
void drain_equal_queues(Q& q1, Q& q2)
{
  while(!q1.empty())
  {
    assert(!q2.empty());

    int id = q1.front_element();
    assert(id == q2.front_element());

    assert(q1.priority(id) == q2.priority(id));
    assert(q1.value(id) == q2.value(id));

    q1.remove_element(id);
    q2.remove_element(id);
  }
  assert(q2.empty());
}

void copy_construct()
{
  indexed_heap_t<int, std::string> q1;

  q1.add_element(1, "1");
  q1.add_element(2, "2");
  q1.add_element(3, "3");

  indexed_heap_t<int, std::string> q2 = q1;

  drain_equal_queues(q1, q2);
}

void move_construct()
{
  indexed_heap_t<int, std::string> q1;

  q1.add_element(1, "1");
  q1.add_element(2, "2");
  q1.add_element(3, "3");

  indexed_heap_t<int, std::string> q2 = q1;
  indexed_heap_t<int, std::string> q3 = std::move(q1);

  assert(q1.empty());
  
  drain_equal_queues(q2, q3);
}

void copy_assign()
{
  indexed_heap_t<int, std::string> q1;

  q1.add_element(1, "1");
  q1.add_element(2, "2");
  q1.add_element(3, "3");

  indexed_heap_t<int, std::string> q2;
  q2 = q1;

  drain_equal_queues(q1, q2);
}

void move_assign()
{
  indexed_heap_t<int, std::string> q1;

  q1.add_element(1, "1");
  q1.add_element(2, "2");
  q1.add_element(3, "3");

  indexed_heap_t<int, std::string> q2 = q1;
  indexed_heap_t<int, std::string> q3;
  q3 = std::move(q1);

  assert(q1.empty());
  
  drain_equal_queues(q2, q3);
}

void swap()
{
  indexed_heap_t<int, std::string> q1;

  q1.add_element(1, "1");
  q1.add_element(2, "2");
  q1.add_element(3, "3");

  indexed_heap_t<int, std::string> q2;

  q2.add_element(4, "4");
  q2.add_element(5, "5");
  q2.add_element(6, "6");

  indexed_heap_t<int, std::string> q3 = q1;
  indexed_heap_t<int, std::string> q4 = q2;

  swap(q3, q4);

  drain_equal_queues(q1, q4);
  drain_equal_queues(q2, q3);
}

} // anonymous

int main()
{
  maxheap();
  minheap();
  duplicate_prios_maxheap();
  duplicate_prios_minheap();
  remove_non_front_ids();

  copy_construct();
  move_construct();
  copy_assign();
  move_assign();
  swap();

  return 0;
}
