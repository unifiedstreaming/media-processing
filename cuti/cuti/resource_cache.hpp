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

#ifndef CUTI_RESOURCE_CACHE_HPP_
#define CUTI_RESOURCE_CACHE_HPP_

#include "chrono_types.hpp"
#include "function.hpp"
#include "mutex_wrapper.hpp"
#include "scoped_guard.hpp"

#include <cassert>
#include <cstddef>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <utility>

namespace cuti
{

/*
 * A resource cache is intended to be used to hold resources that are
 * expensive or time-consuming to create and should therefore be
 * reused if possible.
 *
 * It is parameterized on a key type K (e.g. an IP address combined
 * with a port number) and a resource type R (e.g. a TCP connection).
 * The cache assumes that different instances of R with equal keys
 * provide equivalent functionality and are interchangable.
 *
 * A resource cache is re-entrant; multiple threads may share a single
 * resource cache instance.
 */

struct resource_cache_settings_t
{
  resource_cache_settings_t()
  : max_keys_(16)
  , max_key_age_(minutes_t(4))
  , max_resources_per_key_(64)
  , max_resource_age_(minutes_t(2))
  { }

  // maximum number of keys the cache keeps track of
  // (before evicting the least recently active key)
  std::size_t max_keys_;

  // maximum duration of key inactivity
  // (before a key is considered stale and removed)
  duration_t max_key_age_;

  // maximum number of resources per key the cache keeps track of 
  // (before evicting the least recently stored resource)
  std::size_t max_resources_per_key_;

  // maximum duration a resource is stored
  // (before a resource is considered stale and removed)
  duration_t max_resource_age_;
};

template<typename K, typename R>
struct resource_cache_t
{
  /*
   * Constructs a resource cache.
   *
   * ktor ('key-to-resource') must be a factory functor that returns a
   * non-null std::unique_ptr<R> to a (new) resource for a
   * specific key value of type K.  See the obtain() member function.
   *
   * The rtok ('resource-to-key') functor must return the key (of type
   * K) for an existing resource of type R. See the store() member
   * function.
   */
  template<typename KtoR, typename RtoK>
  resource_cache_t(KtoR&& ktor, RtoK&& rtok,
    resource_cache_settings_t const& settings = resource_cache_settings_t{})
  : ktor_(std::forward<KtoR>(ktor))
  , rtok_(std::forward<RtoK>(rtok))
  , wrapped_impl_(settings)
  {
    assert(ktor_ != nullptr);
    assert(rtok_ != nullptr);
  }

  /*
   * Obtains a resource for a specific key from the cache.  If no such
   * resource is found, a new resource for this key is created by
   * calling the ktor functor that was passed to the cache's
   * constructor.  The caller obtains ownership of the returned
   * resource, and may (or may not) decide to store it back into the
   * cache for later reuse.
   */
  std::unique_ptr<R> obtain(K const& key)
  {
    std::unique_ptr<R> result = nullptr;
    std::list<stored_resource_t> removed{};

    {
      auto impl = wrapped_impl_.lock();
      auto resource_list = impl->try_lock_resource_list(removed, key);
      impl.unlock();

      if(resource_list != nullptr)
      {
        result = resource_list->try_obtain(removed);
      }
    }

    if(result == nullptr)
    {
      result = ktor_(key);
      assert(result != nullptr);
    }

    return result;
  }
      
  /*
   * Stores a resource into the cache for possible later reuse.  The
   * resource's key is determined by calling to rtok functor that was
   * passed to the cache's constructor.  The ownership of the resource
   * is passed to the cache.
   */
  void store(std::unique_ptr<R> resource)
  {
    assert(resource != nullptr);
    auto key = rtok_(*resource);
    std::list<stored_resource_t> removed{};

    {
      auto impl = wrapped_impl_.lock();
      auto resource_list = impl->try_lock_resource_list(removed, key);
      impl.unlock();

      if(resource_list != nullptr)
      {
        resource_list->store(removed, std::move(resource));
      }
    }
  }

  /*
   * Removes all resources for a specific key from the cache and
   * destroys them.  This may be useful if the application logic
   * determines that any currently stored resources for this key are
   * no longer usable and should be re-created.
   */
  void wipe(K const& key)
  {
    std::list<stored_resource_t> removed{};

    {
      auto impl = wrapped_impl_.lock();
      auto resource_list = impl->try_lock_resource_list(removed, key);
      impl.unlock();

      if(resource_list != nullptr)
      {
        resource_list->clear(removed);
      }
    }
  }     

private :
  struct stored_resource_t
  {
    explicit stored_resource_t(time_point_t expiration_time,
                               std::unique_ptr<R> resource)
    : expiration_time_(expiration_time)
    , resource_((assert(resource != nullptr), std::move(resource)))
    { }

    time_point_t expiration_time() const
    { return expiration_time_; }

    std::unique_ptr<R> extract_resource()
    {
      assert(resource_ != nullptr);
      return std::move(resource_);
    }

  private :
    time_point_t const expiration_time_;
    std::unique_ptr<R> resource_;
  };

  struct resource_list_t
  {
    resource_list_t(std::size_t max_resources, duration_t max_resource_age)
    : max_resources_(max_resources)
    , max_resource_age_(max_resource_age)
    , resources_()
    { }

    // returns nullptr if no resource found
    std::unique_ptr<R> try_obtain(std::list<stored_resource_t>& removed)
    {
      this->remove_stale_resources(removed, cuti_clock_t::now());

      std::unique_ptr<R> result = nullptr;
      if(!resources_.empty())
      {
        result = resources_.back().extract_resource();
        resources_.pop_back();
      }

      return result;
    }

    void store(std::list<stored_resource_t>& removed,
               std::unique_ptr<R> resource)
    {
      assert(resource != nullptr);

      auto now = cuti_clock_t::now();
      this->remove_stale_resources(removed, now);

      auto old_n_resources = resources_.size();
      resources_.emplace_back(now + max_resource_age_, std::move(resource));
      if(old_n_resources == max_resources_)
      {
        removed.splice(removed.end(), resources_, resources_.begin());
      }
    }

    void clear(std::list<stored_resource_t>& removed)
    {
      removed.splice(removed.end(),
                     resources_, resources_.begin(), resources_.end());
    }
        
  private :
    void remove_stale_resources(std::list<stored_resource_t>& removed,
                                time_point_t now)
    {
      for(auto pos = resources_.begin();
          pos != resources_.end() && pos->expiration_time() <= now;
          pos = resources_.begin())
      {
        removed.splice(removed.end(), resources_, pos);
      }
    }
      
  private :
    std::size_t const max_resources_;
    duration_t const max_resource_age_;
    std::list<stored_resource_t> resources_;
  };

  struct per_key_t;
  using map_t = std::map<K, per_key_t>;
  using lru_list_t = std::list<typename map_t::iterator>;

  struct per_key_t
  {
    per_key_t(std::size_t max_resources,
              duration_t max_resource_age,
              time_point_t expiration_time)
    : expiration_time_(expiration_time)
    , lru_pos_(std::nullopt)
    , wrapped_resource_list_(max_resources, max_resource_age)
    { }

    void set_expiration_time(time_point_t expiration_time)
    { expiration_time_ = expiration_time; }

    time_point_t expiration_time() const
    { return expiration_time_; }

    // to be called after insertion in lru list
    void set_lru_pos(lru_list_t::iterator pos)
    { lru_pos_ = pos; }

    lru_list_t::iterator lru_pos() const
    {
      assert(lru_pos_.has_value());
      return *lru_pos_;
    }

    mutex_wrapper_lock_t<resource_list_t> lock_resource_list()
    { return wrapped_resource_list_.lock(); }

  private :
    time_point_t expiration_time_;
    std::optional<typename lru_list_t::iterator> lru_pos_;
    mutex_wrapper_t<resource_list_t> wrapped_resource_list_;
  };

  struct impl_t
  {
    explicit impl_t(resource_cache_settings_t const& settings)
    : max_keys_(settings.max_keys_)
    , max_key_age_(settings.max_key_age_)
    , max_resources_per_key_(settings.max_resources_per_key_)
    , max_resource_age_(settings.max_resource_age_)
    , map_()
    , lru_list_()
    { }

    // returns nullptr if no list available for key
    mutex_wrapper_lock_t<resource_list_t>
    try_lock_resource_list(std::list<stored_resource_t>& removed, K const& key)
    {
      auto now = cuti_clock_t::now();

      // remove stale keys
      for(auto oldest = lru_list_.begin();
          oldest != lru_list_.end() &&
            (*oldest)->second.expiration_time() <= now;
          oldest = lru_list_.begin())
      {
        this->remove_element(removed, *oldest);
      }
        
      // find or insert the element in the map
      auto old_n_keys = map_.size();
      auto [map_pos, inserted] = map_.try_emplace(key,
        max_resources_per_key_, max_resource_age_, now + max_key_age_);

      if(inserted)
      {
        {
          // append the element to the lru list...
          auto guard = make_scoped_guard(
            [&map = map_, map_pos]() { map.erase(map_pos); }
          );
          auto lru_pos = lru_list_.insert(lru_list_.end(), map_pos);
          guard.dismiss();

          // ...and record its lru position
          map_pos->second.set_lru_pos(lru_pos);
        }

        if(old_n_keys == max_keys_)
        {
          // evict oldest element
          auto oldest = lru_list_.begin();
          assert(oldest != lru_list_.end());
          this->remove_element(removed, *oldest);

          if(max_keys_ == 0)
          {
            // evicted the only element, so bail out
            return nullptr;
          }
        }
      }
      else
      {
        // update element's expiration time...
        map_pos->second.set_expiration_time(now + max_key_age_);

        // ...and move it to the end of the lru list
        auto lru_pos = map_pos->second.lru_pos();
        lru_list_.splice(lru_list_.end(), lru_list_, lru_pos);
      }

      return map_pos->second.lock_resource_list();
    }

  private :
    void remove_element(std::list<stored_resource_t>& removed_resources,
                        map_t::iterator map_pos)
    {
      {
        // wait for any other thread to let go of the resource list...
        auto resource_list = map_pos->second.lock_resource_list();

        // ..and clear it
        resource_list->clear(removed_resources);
      }

      // remove the element from the lru list...
      auto lru_pos = map_pos->second.lru_pos();
      lru_list_.erase(lru_pos);

      // ...and remove it from the map
      map_.erase(map_pos);
    }
      
  private :
    std::size_t const max_keys_;
    duration_t const max_key_age_;
    std::size_t const max_resources_per_key_;
    duration_t const max_resource_age_;

    map_t map_;
    lru_list_t lru_list_;
  };
    
private :
  function_t<std::unique_ptr<R>(K const&)> const ktor_;
  function_t<K(R const&)> const rtok_;
  mutex_wrapper_t<impl_t> wrapped_impl_;
};

} // cuti

#endif
