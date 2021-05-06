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

#ifndef CUTI_INDEXED_HEAP_HPP_
#define CUTI_INDEXED_HEAP_HPP_

#include <cassert>
#include <functional>
#include <limits>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

#include "scoped_guard.hpp"
#include "system_error.hpp"

namespace cuti
{

/*
 * indexed_heap_t models a priority queue of <Priority, Value>
 * elements.  Each element is identified by a stable, small
 * non-negative integer id.  This id may be used to access the
 * element, or to remove it from the queue, even when it is not the
 * queue's front element.
 *
 * Please note: just as is the case for std::priority_queue,
 * indexed_heap_t's default comparator type (std::less<Priority>)
 * results in a maxheap with the highest priority elements at the
 * front.  Use std::greater<Priority> to obtain a minheap.
 */
template<typename Priority, typename Value,
         typename Cmp = std::less<Priority>>
struct indexed_heap_t
{
  indexed_heap_t()
    noexcept(std::is_nothrow_default_constructible_v<Cmp>)
  : elements_()
  , free_top_(-1)
  , ordering_()
  , cmp_()
  { }

  explicit indexed_heap_t(Cmp const& cmp)
    noexcept(std::is_nothrow_copy_constructible_v<Cmp>)
  : elements_()
  , free_top_(-1)
  , ordering_()
  , cmp_(cmp)
  { }

  explicit indexed_heap_t(Cmp&& cmp)
    noexcept(std::is_nothrow_move_constructible_v<Cmp>)
  : elements_()
  , free_top_(-1)
  , ordering_()
  , cmp_(std::move(cmp))
  { }

  indexed_heap_t(indexed_heap_t const& rhs)
  : elements_(rhs.elements_)
  , free_top_(rhs.free_top_)
  , ordering_(rhs.ordering_)
  , cmp_(rhs.cmp_)
  { }

  indexed_heap_t(indexed_heap_t&& rhs)
    noexcept(std::is_nothrow_move_constructible_v<Cmp>)
  : elements_(std::move(rhs.elements_))
  , free_top_(rhs.free_top_)
  , ordering_(std::move(rhs.ordering_))
  , cmp_(std::move(rhs.cmp_))
  { rhs.clear(); }

  indexed_heap_t& operator=(indexed_heap_t const& rhs)
  {
    indexed_heap_t tmp(rhs);
    this->swap(tmp);
    return *this;
  }

  indexed_heap_t& operator=(indexed_heap_t&& rhs)
    noexcept(std::is_nothrow_move_constructible_v<Cmp> &&
             std::is_nothrow_swappable_v<Cmp>)
  {
    indexed_heap_t tmp(std::move(rhs));
    this->swap(tmp);
    return *this;
  }

  bool empty() const noexcept
  { return ordering_.empty(); }

  void clear() noexcept
  {
    elements_.clear();
    free_top_ = -1;
    ordering_.clear();
  }

  void swap(indexed_heap_t& that)
    noexcept(std::is_nothrow_swappable_v<Cmp>)
  {
    using std::swap;

    swap(this->elements_, that.elements_);
    swap(this->free_top_, that.free_top_);
    swap(this->ordering_, that.ordering_);
    swap(this->cmp_, that.cmp_);
  }

  /*
   * Adds an element to the queue, returning a small, non-negative
   * integer id that identifies the element.  This id remains valid
   * until the element is removed.
   */
  template<typename P, typename V>
  int add_element(P&& priority, V&& value)
  {
    int index = ordering_size();

    // Reserve slot in elements, guarding it for exceptions.
    int id = add_to_elements(index,
      std::forward<P>(priority), std::forward<V>(value));
    auto id_guard =
      make_scoped_guard([&] { remove_from_elements(id); });

    // Add id to the heap, and move it into place.
    ordering_.push_back(id);
    swim(id);

    id_guard.dismiss();
    return id;
  }

  /*
   * Returns the id of one of the highest priority elements.  The
   * relative ordering of elements with equal priority is unspecified.
   * Predcondition: !empty().
   */
  int front_element() const noexcept
  {
    assert(!this->empty());
    int id = ordering_.front();
    assert(valid_id(id));
    return id;
  }

  /*
   * Returns a reference to the priority of the element identified by
   * <id>; this reference is invalidated at the next call to
   * add_element(), or when the element is removed.
   */
  Priority const& priority(int id) const noexcept
  {
    assert(valid_id(id));
    return elements_[id].opt_pv_->first;
  }
    
  /*
   * Returns a reference to the value of the element identified by
   * <id>; this reference is invalidated at the next call to
   * add_element(), or when the element is removed.
   */
  Value const& value(int id) const noexcept
  {
    assert(valid_id(id));
    return elements_[id].opt_pv_->second;
  }
    
  Value& value(int id) noexcept
  {
    assert(valid_id(id));
    return elements_[id].opt_pv_->second;
  }

  /*
   * Removes an arbitrary element from the queue.
   */
  void remove_element(int id) noexcept
  {
    assert(valid_id(id));

    int index = elements_[id].index_;
    assert(valid_index(index));

    // Move the last element in the binary heap to the
    // position of the removed element...
    int last_id = ordering_.back();
    assert(valid_id(last_id));

    ordering_[index] = last_id;
    elements_[last_id].index_ = index;

    // ...remove the element...
    ordering_.pop_back();
    remove_from_elements(id);

    // ...and move the former last element into place.
    if(last_id != id)
    {
      if(!swim(last_id))
      {
        sink(last_id);
      }
    }
  }
    
private :
  bool valid_id(int id) const noexcept
  {
    return id >= 0 &&
           static_cast<unsigned int>(id) < elements_.size() &&
           elements_[id].opt_pv_ != std::nullopt;
  }

  int ordering_size() const noexcept
  {
    return static_cast<int>(ordering_.size());
  }

  bool valid_index(int index) const noexcept
  {
    return index >= 0 && index < ordering_size();
  }
    
  template<typename P, typename V>
  int add_to_elements(int index, P&& priority, V&& value)
  {
    int id = free_top_;

    if(id == -1)
    {
      // Append new element
      constexpr unsigned int max_size = std::numeric_limits<int>::max();
      if(elements_.size() == max_size)
      {
        throw system_exception_t("indexed_heap_t: out of element ids");
      }

      id = static_cast<int>(elements_.size());
      elements_.emplace_back(index,
        std::forward<P>(priority), std::forward<V>(value));
    }        
    else
    {
      // Re-use free slot
      element_t& element = elements_[id];
      assert(element.opt_pv_ == std::nullopt);
      element.opt_pv_.emplace(
        std::forward<P>(priority), std::forward<V>(value));
      free_top_ = element.index_;
      element.index_ = index;
    }

    return id;
  }

  void remove_from_elements(int id) noexcept
  {
    assert(valid_id(id));

    element_t& element = elements_[id];
    element.opt_pv_ = std::nullopt;
    element.index_ = free_top_;
    free_top_ = id;
  }

  /*
   * Move id up as far as needed, returning whether it was moved up at
   * all.
   */
  bool swim(int id) noexcept
  {
    assert(valid_id(id));
    int index = elements_[id].index_;
    assert(valid_index(index));
    
    bool moved = false;
    while(index > 0)
    {
      int parent_index = (index - 1) / 2;
      assert(valid_index(parent_index));
      int parent_id = ordering_[parent_index];
      assert(valid_id(parent_id));

      if(!cmp_(elements_[parent_id].opt_pv_->first,
               elements_[id].opt_pv_->first))
      {
        // parent priority >= priority; done
        break;
      }

      // trade places with parent
      ordering_[index] = parent_id;
      ordering_[parent_index] = id;
      elements_[id].index_ = parent_index;
      elements_[parent_id].index_ = index;

      moved = true;
      index = parent_index;
    }

    return moved;
  }

  /*
   * Move id down as far as needed.
   */
  void sink(int id) noexcept
  {
    assert(valid_id(id));
    int index = elements_[id].index_;
    assert(valid_index(index));

    int limit = ordering_size();
    while(index < limit / 2)
    {
      // assume id has the highest priority until proven otherwise
      int highest_id = id;
      int highest_index = index;

      // check the priorities of index's two potential children,
      // whose indexes are 2 * index + 1 and 2 * index + 2
      for(int child_index = 2 * index + 1;
          child_index < limit && child_index <= 2 * index + 2;
          ++child_index)
      {
        assert(valid_index(child_index));
        int child_id = ordering_[child_index];
        assert(valid_id(child_id));
        
        if(cmp_(elements_[highest_id].opt_pv_->first,
                elements_[child_id].opt_pv_->first))
        {
          // child priority > highest priority
          highest_id = child_id;
          highest_index = child_index;
        }
      }

      if(highest_index == index)
      {
        // no child with higher priority; done
        break;
      }

      // trade places with highest priority child
      ordering_[index] = highest_id;
      ordering_[highest_index] = id;
      elements_[id].index_ = highest_index;
      elements_[highest_id].index_ = index;

      index = highest_index;
    }
  }
      
private :
  struct element_t
  {
    template<typename P, typename V>
    element_t(int index, P&& priority, V&& value)
    : index_(index)
    , opt_pv_(std::in_place,
              std::forward<P>(priority), std::forward<V>(value))
    { }

    int index_; // indexes into ordering_ if in use;
                // next free slot in elements_ otherwise
    std::optional<std::pair<Priority, Value>> opt_pv_;
  };

  std::vector<element_t> elements_; // id is index into elements_
  int free_top_; // stack of free slots in elements_
  std::vector<int> ordering_; // binary heap of ids
  Cmp cmp_;
};

template<typename Priority, typename Value, typename Cmp>
void swap(indexed_heap_t<Priority, Value, Cmp>& q1,
          indexed_heap_t<Priority, Value, Cmp>& q2)
  noexcept(std::is_nothrow_swappable_v<Cmp>)
{
  q1.swap(q2);
}
  
} // cuti

#endif
