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

#include <string>

// Enable assert
#undef NDEBUG
#include <cassert>

namespace // anonymous
{

using namespace cuti;

void maxheap()
{
  indexed_heap_t<int, std::string> q;
  assert(q.empty());

  int id1 = q.add_element(1, "one");
  assert(!q.empty());

  assert(q.front_element() == id1);
  assert(q.priority(id1) == 1);
  assert(q.value(id1) == "one");

  int id2 = q.add_element(2, "two");
  assert(!q.empty());

  assert(q.front_element() == id2);
  assert(q.priority(id2) == 2);
  assert(q.value(id2) == "two");

  int id3 = q.add_element(3, "three");
  assert(!q.empty());

  assert(q.front_element() == id3);
  assert(q.priority(id3) == 3);
  assert(q.value(id3) == "three");

  int id4 = q.add_element(4, "four");
  assert(!q.empty());

  assert(q.front_element() == id4);
  assert(q.priority(id4) == 4);
  assert(q.value(id4) == "four");

  q.remove_element(id4);
  assert(!q.empty());

  assert(q.front_element() == id3);
  assert(q.priority(id3) == 3);
  assert(q.value(id3) == "three");

  q.remove_element(id3);
  assert(!q.empty());

  assert(q.front_element() == id2);
  assert(q.priority(id2) == 2);
  assert(q.value(id2) == "two");

  q.remove_element(id2);
  assert(!q.empty());

  assert(q.front_element() == id1);
  assert(q.priority(id1) == 1);
  assert(q.value(id1) == "one");

  q.remove_element(id1);
  assert(q.empty());
}

void minheap()
{
  indexed_heap_t<int, std::string, std::greater<int>> q;
  assert(q.empty());

  int id1 = q.add_element(1, "one");
  assert(!q.empty());

  assert(q.front_element() == id1);
  assert(q.priority(id1) == 1);
  assert(q.value(id1) == "one");

  int id2 = q.add_element(2, "two");
  assert(!q.empty());

  assert(q.front_element() == id1);
  assert(q.priority(id2) == 2);
  assert(q.value(id2) == "two");

  int id3 = q.add_element(3, "three");
  assert(!q.empty());

  assert(q.front_element() == id1);
  assert(q.priority(id3) == 3);
  assert(q.value(id3) == "three");

  int id4 = q.add_element(4, "four");
  assert(!q.empty());

  assert(q.front_element() == id1);
  assert(q.priority(id4) == 4);
  assert(q.value(id4) == "four");

  q.remove_element(id4);
  assert(!q.empty());

  assert(q.front_element() == id1);
  assert(q.priority(id3) == 3);
  assert(q.value(id3) == "three");

  q.remove_element(id3);
  assert(!q.empty());

  assert(q.front_element() == id1);
  assert(q.priority(id2) == 2);
  assert(q.value(id2) == "two");

  q.remove_element(id2);
  assert(!q.empty());

  assert(q.front_element() == id1);
  assert(q.priority(id1) == 1);
  assert(q.value(id1) == "one");

  q.remove_element(id1);
  assert(q.empty());
}

void duplicate_prios_maxheap()
{
  indexed_heap_t<int, std::string> q;
  assert(q.empty());

  int id11 = q.add_element(1, "11");
  assert(!q.empty());

  assert(q.front_element() == id11);
  assert(q.priority(id11) == 1);
  assert(q.value(id11) == "11");

  int id12 = q.add_element(1, "12");
  assert(!q.empty());

  assert(q.front_element() == id11 || q.front_element() == id12);
  assert(q.priority(id12) == 1);
  assert(q.value(id12) == "12");

  int id21 = q.add_element(2, "21");
  assert(!q.empty());

  assert(q.front_element() == id21);
  assert(q.priority(id21) == 2);
  assert(q.value(id21) == "21");

  int id22 = q.add_element(2, "22");
  assert(!q.empty());

  assert(q.front_element() == id21 || q.front_element() == id22);
  assert(q.priority(id22) == 2);
  assert(q.value(id22) == "22");

  q.remove_element(id22);
  assert(!q.empty());

  assert(q.front_element() == id21);
  assert(q.priority(id21) == 2);
  assert(q.value(id21) == "21");

  q.remove_element(id21);
  assert(!q.empty());

  assert(q.front_element() == id11 || q.front_element() == id12);
  assert(q.priority(id12) == 1);
  assert(q.value(id12) == "12");

  q.remove_element(id12);
  assert(!q.empty());

  assert(q.front_element() == id11);
  assert(q.priority(id11) == 1);
  assert(q.value(id11) == "11");

  q.remove_element(id11);
  assert(q.empty());
}
  
void duplicate_prios_minheap()
{
  indexed_heap_t<int, std::string, std::greater<int>> q;
  assert(q.empty());

  int id11 = q.add_element(1, "11");
  assert(!q.empty());

  assert(q.front_element() == id11);
  assert(q.priority(id11) == 1);
  assert(q.value(id11) == "11");

  int id12 = q.add_element(1, "12");
  assert(!q.empty());

  assert(q.front_element() == id11 || q.front_element() == id12);
  assert(q.priority(id12) == 1);
  assert(q.value(id12) == "12");

  int id21 = q.add_element(2, "21");
  assert(!q.empty());

  assert(q.front_element() == id11 || q.front_element() == id12);
  assert(q.priority(id21) == 2);
  assert(q.value(id21) == "21");

  int id22 = q.add_element(2, "22");
  assert(!q.empty());

  assert(q.front_element() == id11 || q.front_element() == id12);
  assert(q.priority(id22) == 2);
  assert(q.value(id22) == "22");

  q.remove_element(id22);
  assert(!q.empty());

  assert(q.front_element() == id11 || q.front_element() == id12);
  assert(q.priority(id21) == 2);
  assert(q.value(id21) == "21");

  q.remove_element(id21);
  assert(!q.empty());

  assert(q.front_element() == id11 || q.front_element() == id12);
  assert(q.priority(id12) == 1);
  assert(q.value(id12) == "12");

  q.remove_element(id12);
  assert(!q.empty());

  assert(q.front_element() == id11);
  assert(q.priority(id11) == 1);
  assert(q.value(id11) == "11");

  q.remove_element(id11);
  assert(q.empty());
}

void four_layers()
{
  // known to cause a decent number of left and right sinks

  indexed_heap_t<int, std::string> q;
  assert(q.empty());

  std::vector<int> ids;
  for(int i = 0; i < 15; ++i)
  {
    int id = q.add_element(i, std::to_string(i));
    assert(!q.empty());
    assert(q.front_element() == id);
    assert(q.priority(id) == i);
    assert(q.value(id) == std::to_string(i));

    ids.push_back(id);
  }

  while(!ids.empty())
  {
    int id = ids.back();
    ids.pop_back();
    int i = static_cast<int>(ids.size());

    assert(!q.empty());
    assert(q.front_element() == id);
    assert(q.priority(id) == i);
    assert(q.value(id) == std::to_string(i));
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

  four_layers();

  copy_construct();
  move_construct();
  copy_assign();
  move_assign();
  swap();

  return 0;
}
