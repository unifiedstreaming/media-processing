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

#ifndef CUTI_FUNCTION_HPP_
#define CUTI_FUNCTION_HPP_

#include <cassert>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

namespace cuti
{

/*
 * A stopgap replacement for std::function that does not suffer from
 * const correctness-related race conditions.  Enforces deep-const by
 * simply requiring that the wrapped callable is const-callable (and
 * copyable).
 */

template<typename Signature>
struct function_t;

template<typename R, typename... Args>
struct function_t<R(Args...)>
{
  function_t() noexcept
  : impl_(nullptr)
  { }

  function_t(std::nullptr_t) noexcept
  : impl_(nullptr)
  { }

  template<typename F>
  function_t(F* f)
  : impl_(f == nullptr ? nullptr : std::make_unique<impl_t<F*>>(f))
  { }

  template<typename F, typename = std::enable_if_t<
    !std::is_convertible_v<std::decay_t<F>*, function_t const*>>>
  function_t(F&& f) 
  : impl_(std::make_unique<impl_t<std::decay_t<F>>>(std::forward<F>(f)))
  { }

  function_t(function_t const& rhs)
  : impl_(rhs.impl_ == nullptr ? nullptr : rhs.impl_->clone())
  { }

  function_t(function_t&&) = default;

  function_t& operator=(function_t const& rhs)
  {
    function_t tmp(rhs);
    swap(*this, tmp);
    return *this;
  }

  function_t& operator=(function_t&&) = default;

  explicit operator bool() const noexcept
  { return impl_ != nullptr; }
  
  R operator()(Args... args) const
  {
    assert(impl_ != nullptr);
    
    if constexpr(std::is_void_v<R>)
    {
      impl_->invoke(std::forward<Args>(args)...);
    }
    else
    {
      return impl_->invoke(std::forward<Args>(args)...);
    }
  }

  ~function_t() = default;

  friend bool operator==(function_t const& lhs, std::nullptr_t) noexcept
  { return lhs.impl_ == nullptr; }
  
  friend bool operator==(std::nullptr_t, function_t const& rhs) noexcept
  { return rhs.impl_ == nullptr; }
  
  friend bool operator!=(function_t const& lhs, std::nullptr_t) noexcept
  { return lhs.impl_ != nullptr; }
  
  friend bool operator!=(std::nullptr_t, function_t const& rhs) noexcept
  { return rhs.impl_ != nullptr; }

  friend void swap(function_t& f1, function_t& f2) noexcept
  {
    using namespace std;
    swap(f1.impl_, f2.impl_);
  }
    
private :
  struct abstract_impl_t
  {
    abstract_impl_t() = default;

    abstract_impl_t(abstract_impl_t const&) = delete;
    abstract_impl_t& operator=(abstract_impl_t const&) = delete;

    virtual R invoke(Args... args) const = 0;
    virtual std::unique_ptr<abstract_impl_t> clone() const = 0;

    virtual ~abstract_impl_t() = default;
  };

  template<typename F>
  struct impl_t : abstract_impl_t
  {
    template<typename FF, typename = std::enable_if_t<
      !std::is_convertible_v<std::decay_t<FF>*, impl_t const*>>>
    explicit impl_t(FF&& ff)
    : abstract_impl_t()
    , f_(std::forward<FF>(ff))
    {
      if constexpr(std::is_pointer_v<F>)
      {
        assert(f_ != nullptr);
      }
    }

    R invoke(Args... args) const override
    {
      if constexpr(std::is_void_v<R>)
      {
        f_(std::forward<Args>(args)...);
      }
      else
      {
        return f_(std::forward<Args>(args)...);
      }
    }
     
    std::unique_ptr<abstract_impl_t> clone() const override
    {
      return std::make_unique<impl_t>(f_);
    }

  private :
    F f_;
  };

private :
  std::unique_ptr<abstract_impl_t const> impl_;
};

} // cuti

#endif
