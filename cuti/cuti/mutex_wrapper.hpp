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

#ifndef CUTI_MUTEX_WRAPPER_HPP_
#define CUTI_MUTEX_WRAPPER_HPP_

#include <cassert>
#include <cstddef>
#include <mutex>
#include <type_traits>
#include <utility>

namespace cuti
{

template<typename T>
struct mutex_wrapper_t;

template<typename T>
struct mutex_wrapper_lock_t
{
  mutex_wrapper_lock_t() noexcept
  : lock_()
  , data_(nullptr)
  { }

  mutex_wrapper_lock_t(std::nullptr_t) noexcept
  : lock_()
  , data_(nullptr)
  { }

  mutex_wrapper_lock_t(mutex_wrapper_lock_t&& rhs) noexcept
  : lock_(std::move(rhs.lock_))
  , data_(rhs.data_)
  {
    rhs.data_ = nullptr;
  }

  template<typename U, typename = std::enable_if_t<
    std::is_convertible_v<U*, T*>
  >>
  mutex_wrapper_lock_t(mutex_wrapper_lock_t<U>&& rhs) noexcept
  : lock_(std::move(rhs.lock_))
  , data_(rhs.data_)
  {
    rhs.data_ = nullptr;
  }

  mutex_wrapper_lock_t& operator=(mutex_wrapper_lock_t&& rhs) noexcept
  {
    mutex_wrapper_lock_t tmp{std::move(rhs)};
    this->swap(tmp);
    return *this;
  }

  template<typename U, typename = std::enable_if_t<
    std::is_convertible_v<U*, T*>
  >>
  mutex_wrapper_lock_t& operator=(mutex_wrapper_lock_t<U>&& rhs) noexcept
  {
    mutex_wrapper_lock_t tmp{std::move(rhs)};
    this->swap(tmp);
    return *this;
  }

  explicit operator bool() const noexcept
  { return data_ != nullptr; }

  bool operator==(std::nullptr_t) const noexcept
  { return data_ == nullptr; }

  T* operator->() const noexcept
  {
    assert(*this != nullptr);
    return data_;
  }

  T& operator*() const noexcept
  {
    assert(*this != nullptr);
    return *data_;
  }

  void unlock() noexcept
  { *this = mutex_wrapper_lock_t{}; }

  void swap(mutex_wrapper_lock_t& that) noexcept
  {
    using std::swap;

    swap(this->lock_, that.lock_);
    swap(this->data_, that.data_);
  }

  friend
  void swap(mutex_wrapper_lock_t& lhs, mutex_wrapper_lock_t& rhs) noexcept
  {
    lhs.swap(rhs);
  }
    
private :
  template<typename U>
  friend struct mutex_wrapper_lock_t;

  template<typename U>
  friend struct mutex_wrapper_t;
  
  mutex_wrapper_lock_t(std::mutex& mut, T& data)
  : lock_(mut)
  , data_(&data)
  { }

private :
  std::unique_lock<std::mutex> lock_;
  T* data_;
};

template<typename T>
struct mutex_wrapper_t
{
  using lock_t = mutex_wrapper_lock_t<T>;
  using const_lock_t = mutex_wrapper_lock_t<T const>;
  
  mutex_wrapper_t()
  : mut_()
  , data_()
  { }

  template<typename Arg, typename = std::enable_if_t<
    !std::is_convertible_v<std::decay_t<Arg>*, mutex_wrapper_t const*>
  >>
  explicit mutex_wrapper_t(Arg&& arg)
  : mut_()
  , data_(std::forward<Arg>(arg))
  { }

  template<typename Arg0, typename Arg1, typename... ArgN>
  mutex_wrapper_t(Arg0&& arg0, Arg1&& arg1, ArgN&&... argN)
  : mut_()
  , data_(std::forward<Arg0>(arg0),
          std::forward<Arg1>(arg1),
          std::forward<ArgN>(argN)...)
  { }

  lock_t lock() &
  { return lock_t{mut_, data_}; }

  const_lock_t lock() const &
  { return const_lock_t{mut_, data_}; }

  // prevent locking on an expiring wrapper
  lock_t lock() && = delete;
  const_lock_t lock() const && = delete;

private :
  std::mutex mutable mut_;
  T data_;
};

} // cuti

#endif
