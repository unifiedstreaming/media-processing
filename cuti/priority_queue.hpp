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

#ifndef CUTI_PRIORITY_QUEUE_HPP_
#define CUTI_PRIORITY_QUEUE_HPP_

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
 * The cuti priority queue.
 *
 * Unlike std::priority_queue, adding an element to a cuti priority
 * queue returns a small, non-negative stable integer id that
 * identifies the element in the queue.  This id may be used to access
 * the element, or to remove it from the queue, even when it is not
 * the queue's front element.  Furthermore, in addition to its
 * priority, each element also holds a modifiable value of some
 * arbitrary type.
 *
 * Please note: just as specified for std::priority_queue, the default
 * comparator type, std::less<Priority>, results in a maxheap with the
 * highest priority elements at the front.  Use std::greater<Priority>
 * to obtain a minheap.
 */
template<typename Priority, typename Value,
         typename Cmp = std::less<Priority>>
struct priority_queue_t
{
  priority_queue_t()
    noexcept(std::is_nothrow_default_constructible_v<Cmp>)
  : elements_()
  , free_top_(-1)
  , ordering_()
  , cmp_()
  { }

  explicit priority_queue_t(Cmp const& cmp)
    noexcept(std::is_nothrow_copy_constructible_v<Cmp>)
  : elements_()
  , free_top_(-1)
  , ordering_()
  , cmp_(cmp)
  { }

  explicit priority_queue_t(Cmp&& cmp)
    noexcept(std::is_nothrow_move_constructible_v<Cmp>)
  : elements_()
  , free_top_(-1)
  , ordering_()
  , cmp_(std::move(cmp))
  { }

  priority_queue_t(priority_queue_t const& rhs)
  : elements_(rhs.elements_)
  , free_top_(rhs.free_top_)
  , ordering_(rhs.ordering_)
  , cmp_(rhs.cmp_)
  { }

  priority_queue_t(priority_queue_t&& rhs)
    noexcept(std::is_nothrow_move_constructible_v<Cmp>)
  : elements_(std::move(rhs.elements_))
  , free_top_(rhs.free_top_)
  , ordering_(std::move(rhs.ordering_))
  , cmp_(std::move(rhs.cmp_))
  { rhs.clear(); }

  priority_queue_t& operator=(priority_queue_t const& rhs)
  {
    priority_queue_t tmp(rhs);
    this->swap(tmp);
    return *this;
  }

  priority_queue_t& operator=(priority_queue_t&& rhs)
    noexcept(std::is_nothrow_move_constructible_v<Cmp> &&
             std::is_nothrow_swappable_v<Cmp>)
  {
    priority_queue_t tmp(std::move(rhs));
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

  void swap(priority_queue_t& that)
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
        throw system_exception_t("priority_queue_t: out of element ids");
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
   * Move id down as deep as possible.
   */
  void sink(int id) noexcept
  {
    assert(valid_id(id));
    int index = elements_[id].index_;
    assert(valid_index(index));

    int limit = ordering_size();
    while(index < limit / 2 && 2 * index + 1 < limit)
    {
      // we have a left child
      int left_index = 2 * index + 1;
      assert(valid_index(left_index));
      int left_id = ordering_[left_index];
      assert(valid_id(left_id));

      // assume id has the highest priority until proven otherwise
      int highest_id = id;
      int highest_index = index;

      if(left_index + 1 < limit)
      {
        // we have a right child
        int right_index = left_index + 1;
        assert(valid_index(right_index));
        int right_id = ordering_[right_index];
        assert(valid_id(right_id));

        if(!cmp_(elements_[right_id].opt_pv_->first,
                 elements_[id].opt_pv_->first))
        {
          // right child priority >= priority
          highest_id = right_id;
          highest_index = right_index;
        }
      }

      if(!cmp_(elements_[left_id].opt_pv_->first,
               elements_[highest_id].opt_pv_->first))
      {
        // left child priority >= highest priority
        highest_id = left_id;
        highest_index = left_index;
      }

      if(id == highest_id)
      {
        // children have lower priorities; done
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
void swap(priority_queue_t<Priority, Value, Cmp>& q1,
          priority_queue_t<Priority, Value, Cmp>& q2)
  noexcept(std::is_nothrow_swappable_v<Cmp>)
{
  q1.swap(q2);
}
  
} // cuti

#endif
