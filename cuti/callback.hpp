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

#ifndef CUTI_CALLBACK_HPP_
#define CUTI_CALLBACK_HPP_

#include "linkage.h"

#include <cassert>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

namespace cuti
{

/*
 * Type-erased callback wrapper as a workaround for some of the
 * problems of std::function<void()>.
 */
struct CUTI_ABI callback_t
{
  callback_t() noexcept
  : impl_(nullptr)
  { }

  callback_t(std::nullptr_t) noexcept
  : impl_(nullptr)
  { }

  template<typename F, typename = std::enable_if_t<
    !std::is_convertible_v<std::decay_t<F>*, callback_t const*>>>
  callback_t(F&& f)
  : impl_(std::make_shared<impl_instance_t<std::decay_t<F>>>(
      std::forward<F>(f)))
  { }

  callback_t(callback_t const& rhs) noexcept
  : impl_(rhs.impl_)
  { }

  callback_t(callback_t&& rhs) noexcept
  : impl_(std::move(rhs.impl_))
  { }

  callback_t& operator=(std::nullptr_t) noexcept
  {
    impl_ = nullptr;
    return *this;
  }

  template<typename F, typename = std::enable_if_t<
    !std::is_convertible_v<std::decay_t<F>*, callback_t const*>>>
  callback_t& operator=(F&& f)
  {
    impl_ = std::make_shared<impl_instance_t<std::decay_t<F>>>(
      std::forward<F>(f));
    return *this;
  }

  callback_t& operator=(callback_t const& rhs) noexcept
  {
    impl_ = rhs.impl_;
    return *this;
  }

  callback_t& operator=(callback_t&& rhs) noexcept
  {
    impl_ = std::move(rhs.impl_);
    return *this;
  }

  explicit operator bool() const noexcept
  {
    return impl_ != nullptr;
  }

  void swap(callback_t& other) noexcept
  {
    impl_.swap(other.impl_);
  }

  void operator()() const
  {
    assert(*this != nullptr);
    (*impl_)();
  }

  friend bool operator==(callback_t const& lhs, std::nullptr_t) noexcept
  {
    return !lhs.operator bool();
  }

  friend bool operator==(std::nullptr_t, callback_t const& rhs) noexcept
  {
    return !rhs.operator bool();
  }

  friend bool operator!=(callback_t const& lhs, std::nullptr_t) noexcept
  {
    return lhs.operator bool();
  }

  friend bool operator!=(std::nullptr_t, callback_t const& rhs) noexcept
  {
    return rhs.operator bool();
  }

private :
  struct CUTI_ABI impl_t
  {
    impl_t()
    { }

    impl_t(impl_t const&) = delete;
    impl_t& operator=(impl_t const&) = delete;

    virtual void operator()() const = 0;

    virtual ~impl_t();
  };

  template<typename F>
  struct impl_instance_t : impl_t
  {
    impl_instance_t(F const& f)
    : impl_t()
    , f_(f)
    { }

    impl_instance_t(F&& f)
    : impl_t()
    , f_(std::move(f))
    { }

    void operator()() const override
    {
      f_();
    }

  private :
    F f_;
  };

private :
  std::shared_ptr<impl_t const> impl_;
};

CUTI_ABI
inline void swap(callback_t& cb1, callback_t& cb2) noexcept
{
  cb1.swap(cb2);
}

} // cuti

#endif
