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

#ifndef CUTI_LIST_ARENA_HPP_
#define CUTI_LIST_ARENA_HPP_

#include "system_error.hpp"

#include <cassert>
#include <optional>
#include <limits>
#include <utility>
#include <vector>

namespace cuti
{

/*
 * A list arena is a special-purpose, tightly packed container of
 * doubly-linked lists of a single element type.  These lists, as well
 * as their elements, are identified by small non-negative integer
 * ids, which are just indexes into an underlying array.  Although
 * adding a new list or element to the arena may cause existing
 * elements to move to a different memory location, their ids remain
 * stable.
 *
 * In addition to the ids for denoting actual elements, each list also
 * has a specific past-the-end id for the position just after its last
 * element.
 *
 * At any given point in time, each element in the arena is a member
 * of exactly one list.  Within a single arena, elements can be
 * rearranged freely, changing list membership when moved before an
 * arena element that is on a different list.
 *
 * Removing an element from the arena does not require the user to
 * specify which list it is on; removing a list from the arena
 * implictly removes all of the list's member elements.
 *
 * An important design consideration is to keep the range of ids
 * small: it starts at 0, and the ids of removed elements and lists,
 * which stand for free slots in the underlying array, are
 * aggressively recycled.  This allows others to use these ids as
 * indexes into their own arrays without the need for an extra mapping
 * layer.
 *
 * Because of its special-purpose nature, the list arena interface was
 * deliberately designed to be different from the interface of the
 * standard C++ containers.
 */

template<typename T>
struct list_arena_t
{
  list_arena_t() noexcept
  : nodes_()
  , free_top_(-1)
  { }

  list_arena_t(list_arena_t const& rhs)
  : nodes_(rhs.nodes_)
  , free_top_(rhs.free_top_)
  { }

  list_arena_t(list_arena_t&& rhs) noexcept
  : nodes_(std::move(rhs.nodes_))
  , free_top_(rhs.free_top_)
  {
    rhs.nodes_.clear();
    rhs.free_top_ = -1;
  }

  list_arena_t& operator=(list_arena_t const& rhs)
  {
    list_arena_t tmp(rhs);
    this->swap(tmp);
    return *this;
  }

  list_arena_t& operator=(list_arena_t&& rhs) noexcept
  {
    list_arena_t tmp(std::move(rhs));
    this->swap(tmp);
    return *this;
  }

  void swap(list_arena_t& that) noexcept
  {
    using std::swap;

    swap(this->nodes_, that.nodes_);
    swap(this->free_top_, that.free_top_);
  }

  /*
   * Tells if <list> is empty.
   */
  bool list_empty(int list) const noexcept
  {
    return first(list) == last(list);
  }

  /*
   * Returns <list>'s first element id.  For an empty list, this is
   * its past-the-end id.
   */
  int first(int list) const noexcept
  {
    assert(is_list(list));
    return nodes_[list].next_;
  }

  /*
   * Returns <list>'s past-the end id.  This id does not denote an
   * actual element.
   */
  int last(int list) const noexcept
  {
    assert(is_list(list));
    return list;
  }

  /*
   * Returns <element>'s next element id; <element> must not be the
   * past-the-end id of its list.
   */
  int next(int element) const noexcept
  {
    assert(is_element(element));
    int result = nodes_[element].next_;
    assert(is_valid(result));
    return result;
  }

  /*
   * Returns <element>'s previous element id; <element> must not be
   * first element id of its list.
   */
  int prev(int element) const noexcept
  {
    assert(is_valid(element));
    int result = nodes_[element].prev_;
    assert(is_element(result));
    return result;
  }

  /*
   * Returns a reference to the value of <element>; <element> must not
   * be the past-the-end id of its list.  The next call to add_list()
   * or add_element_before() invalidates this reference.
   */
  T& value(int element) noexcept
  {
    assert(is_element(element));
    return *nodes_[element].data_;
  }

  T const& value(int element) const noexcept
  {
    assert(is_element(element));
    return *nodes_[element].data_;
  }

  /*
   * Adds a new empty list to the arena, returning its id.
   */
  int add_list()
  {
    int list = free_top_;
    if(list != -1)
    {
      // pop list node from the free list
      free_top_ = nodes_[list].next_;
      nodes_[list].prev_ = list;
      nodes_[list].next_ = list;
    }
    else
    {
      // add new list node
      list = size();
      if(list == max_size)
      {
        throw system_exception_t("list_arena_t: out of node ids");
      }
      nodes_.emplace_back(list);
    }

    return list;
  }

  /*
   * Adds a new element to the arena, initializing its value with
   * <args>, placing it before <before> on <before>'s list, and
   * returning its id.  <before> may or may not be the past-the-end id
   * of its list.
   */
  template<typename... Args>
  int add_element_before(int before, Args&&... args)
  {
    assert(is_valid(before));

    int prev = nodes_[before].prev_;
    int next = before;

    int element = free_top_;
    if(element != -1)
    {
      // initialize and pop data node from the free list
      nodes_[element].data_.emplace(std::forward<Args>(args)...);
      free_top_ = nodes_[element].next_;
      nodes_[element].prev_ = prev;
      nodes_[element].next_ = next;
    }
    else
    {
      // add new data node
      element = size();
      if(element == max_size)
      {
        throw system_exception_t("list_arena_t: out of node ids");
      }
      nodes_.emplace_back(prev, next, std::forward<Args>(args)...);
    }

    nodes_[prev].next_ = element;
    nodes_[next].prev_ = element;

    return element;
  }

  /*
   * Moves <element> to <before>'s list, before <before>.  <element>
   * must not be the past-the-end id of its list; <before> may or may
   * not be the past-the-end id of its list.
   */
  void move_element_before(int before, int element) noexcept
  {
    assert(is_valid(before));
    assert(is_element(element));

    // unlink from the old neighbours...
    int old_prev = nodes_[element].prev_;
    int old_next = nodes_[element].next_;
    nodes_[old_prev].next_ = old_next;
    nodes_[old_next].prev_ = old_prev;

    // ...and link to the new ones
    int new_prev = nodes_[before].prev_;
    int new_next = nodes_[new_prev].next_; // != before when element == before
    nodes_[new_prev].next_ = element;
    nodes_[element].prev_ = new_prev;
    nodes_[element].next_ = new_next;
    nodes_[new_next].prev_ = element;
  }

  /*
   * Removes <element> from the arena.  <element> must not be the
   * past-the-end id of its list.
   */
  void remove_element(int element) noexcept
  {
    assert(is_element(element));

    // unlink element...
    int prev = nodes_[element].prev_;
    int next = nodes_[element].next_;
    nodes_[prev].next_ = next;
    nodes_[next].prev_ = prev;

    // ..and push it on the free list
    nodes_[element].prev_ = -1;
    nodes_[element].next_ = free_top_;
    nodes_[element].data_ = std::nullopt;
    free_top_ = element;
  }

  /*
   * Removes <list>, including all of its elements, from the arena.
   */
  void remove_list(int list) noexcept
  {
    assert(is_list(list));

    // first remove lists's elements...
    for(int element = first(list);
        element != last(list);
        element = first(list))
    {
      remove_element(element);
    }

    // ...then push it on the free list
    nodes_[list].prev_ = -1;
    nodes_[list].next_ = free_top_;
    free_top_ = list;
  }

private :
  static int const max_size = std::numeric_limits<int>::max();

  int size() const noexcept
  {
    return static_cast<int>(nodes_.size());
  }

  bool is_valid(int id) const noexcept
  {
    return id >= 0 && id < size() && nodes_[id].prev_ != -1;
  }

  bool is_list(int id) const noexcept
  {
    return is_valid(id) && !nodes_[id].data_.has_value();
  }

  bool is_element(int id) const noexcept
  {
    return is_valid(id) && nodes_[id].data_.has_value();
  }

private :
  struct node_t
  {
    // constructs a list node
    explicit node_t(int id)
    : prev_(id)
    , next_(id)
    , data_(std::nullopt)
    { }

    // constructs a data node
    template<typename... Args>
    node_t(int prev, int next, Args&&... args)
    : prev_(prev)
    , next_(next)
    , data_(std::in_place, std::forward<Args>(args)...)
    { }

    int prev_;
    int next_;
    std::optional<T> data_;
  };

  std::vector<node_t> nodes_;
  int free_top_;  // singly-linked stack of free nodes
};

template<typename T>
void swap(list_arena_t<T>& a1, list_arena_t<T>& a2) noexcept
{
  a1.swap(a2);
}

} // cuti

#endif
