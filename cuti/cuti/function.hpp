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

#include "type_traits.hpp"

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
 * simply requiring that the wrapped callable is const-callable.
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

  template<typename F, typename = std::enable_if_t<
    !std::is_convertible_v<std::decay_t<F>*, function_t const*>>>
  function_t(F&& f)
  : impl_(is_null(f) ?
          nullptr :
          std::make_shared<impl_t<std::decay_t<F>>>(std::forward<F>(f)))
  { }

  explicit operator bool() const noexcept
  { return impl_ != nullptr; }

  bool operator==(std::nullptr_t) const noexcept
  { return impl_ == nullptr; }
  
  R operator()(Args... args) const
  {
    assert(impl_ != nullptr);

    if constexpr(std::is_same_v<R, void>)
    {
      (*impl_)(std::forward<Args>(args)...);
    }
    else
    {
      return (*impl_)(std::forward<Args>(args)...);
    }
  }

  friend void swap(function_t& f1, function_t& f2) noexcept
  {
    using namespace std;
    swap(f1.impl_, f2.impl_);
  }

private :
  template<typename F>
  static bool is_null(F const& f)
  {
    if constexpr(std::is_function_v<F>)
    {
      return false;
    }
    else if constexpr(!is_equality_comparable_v<F, std::nullptr_t>)
    {
      return false;
    }
    else
    {
      return f == nullptr;
    }
  }

  struct abstract_impl_t
  {
    abstract_impl_t() = default;

    abstract_impl_t(abstract_impl_t const&) = delete;
    abstract_impl_t& operator=(abstract_impl_t const&) = delete;

    virtual R operator()(Args... args) const = 0;

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
      assert(!is_null(f_));
    }

    R operator()(Args... args) const override
    {
      /*
       * Calling f_ with std::invoke() is supposed to enable the use
       * of pointer to member types as callables, but causes weird g++
       * -Warray-bounds warnings from the bowels of <tuple> at -O2 or
       * higher.
       */
      if constexpr(std::is_same_v<R, void>)
      {
        f_(std::forward<Args>(args)...);
      }
      else
      {
        return f_(std::forward<Args>(args)...);
      }
    }
     
  private :
    F f_;
  };

private :
  std::shared_ptr<abstract_impl_t const> impl_;
};

} // cuti

#endif
