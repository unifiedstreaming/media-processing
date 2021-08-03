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

#ifndef CUTI_HANDLER_HPP_
#define CUTI_HANDLER_HPP_

#include "linkage.h"

#include <cassert>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

namespace cuti
{

/*
 * Type-erased wrapper for constant function objects, which (by
 * definition) may be invoked more than once.
 */
struct CUTI_ABI handler_t
{
  handler_t() noexcept
  : impl_(nullptr)
  { }

  handler_t(std::nullptr_t) noexcept
  : impl_(nullptr)
  { }

  template<typename F>
  handler_t(F* f)
  : impl_(f != nullptr ? std::make_shared<impl_instance_t<F*>>(f) : nullptr)
  { }

  template<typename F, typename = std::enable_if_t<
    !std::is_convertible_v<std::decay_t<F>*, handler_t const*>>>
  handler_t(F&& f)
  : impl_(std::make_shared<impl_instance_t<std::decay_t<F>>>(
      std::forward<F>(f)))
  { }

  handler_t(handler_t const& rhs) noexcept
  : impl_(rhs.impl_)
  { }

  handler_t(handler_t&& rhs) noexcept
  : impl_(std::move(rhs.impl_))
  { }

  handler_t& operator=(std::nullptr_t) noexcept
  {
    impl_ = nullptr;
    return *this;
  }

  template<typename F>
  handler_t& operator=(F* f)
  {
    impl_ = f != nullptr ? std::make_shared<impl_instance_t<F*>>(f) : nullptr;
    return *this;
  }

  template<typename F, typename = std::enable_if_t<
    !std::is_convertible_v<std::decay_t<F>*, handler_t const*>>>
  handler_t& operator=(F&& f)
  {
    impl_ = std::make_shared<impl_instance_t<std::decay_t<F>>>(
      std::forward<F>(f));
    return *this;
  }

  handler_t& operator=(handler_t const& rhs) noexcept
  {
    impl_ = rhs.impl_;
    return *this;
  }

  handler_t& operator=(handler_t&& rhs) noexcept
  {
    impl_ = std::move(rhs.impl_);
    return *this;
  }

  explicit operator bool() const noexcept
  {
    return impl_ != nullptr;
  }

  void swap(handler_t& other) noexcept
  {
    impl_.swap(other.impl_);
  }

  void operator()() const
  {
    assert(*this != nullptr);
    (*impl_)();
  }

  friend bool operator==(handler_t const& lhs, std::nullptr_t) noexcept
  {
    return !lhs.operator bool();
  }

  friend bool operator==(std::nullptr_t, handler_t const& rhs) noexcept
  {
    return !rhs.operator bool();
  }

  friend bool operator!=(handler_t const& lhs, std::nullptr_t) noexcept
  {
    return lhs.operator bool();
  }

  friend bool operator!=(std::nullptr_t, handler_t const& rhs) noexcept
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
inline void swap(handler_t& hnd1, handler_t& hnd2) noexcept
{
  hnd1.swap(hnd2);
}

} // cuti

#endif
