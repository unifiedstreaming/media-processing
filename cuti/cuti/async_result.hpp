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

#ifndef CUTI_ASYNC_RESULT_HPP_
#define CUTI_ASYNC_RESULT_HPP_

#include "linkage.h"

#include <cassert>
#include <exception>
#include <utility>
#include <variant>

namespace cuti
{

/*
 * An async_result_t stores the final result of an async operation.
 */
template<typename T>
struct async_result_t
{
  async_result_t()
  : state_()
  { }

  /*
   * Tells if the operation has completed, either by success or
   * exception.
   */
  bool available() const noexcept
  {
    return state_.index() != 0;
  }

  /*
   * Returns any exception produced by the operation, or nullptr if the
   * operation succeeded.
   * PRE: this->available().
   */
  std::exception_ptr exception() const noexcept
  {
    assert(this->available());
    return state_.index() != 2 ? nullptr : std::get<2>(state_);
  }

  /*
   * Returns the value produced by the operation, throwing if the
   * operation produced an exception.
   * PRE: this->available().
   */
  T const& value() const
  {
    assert(this->available());
    if(auto exptr = this->exception())
    {
      std::rethrow_exception(exptr);
    }
    return std::get<1>(state_);
  }
    
  /*
   * Returns the value produced by the operation, throwing if the
   * operation produced an exception.
   * PRE: this->available().
   */
  T& value()
  {
    assert(this->available());
    if(auto exptr = this->exception())
    {
      std::rethrow_exception(exptr);
    }
    return std::get<1>(state_);
  }
    
  /*
   * Called when the operation successfully produces a value.
   */
  template<typename TT>
  void on_success(TT&& value)
  {
    assert(!this->available());
    state_.template emplace<1>(std::forward<TT>(value));
  }

  /*
   * Called when the operation produces an exception.
   */
  template<typename Exptr>
  void on_exception(Exptr&& exptr) noexcept
  {
    assert(exptr != nullptr);
    assert(!this->available());
    state_.template emplace<2>(std::forward<Exptr>(exptr));
  }

private :
  std::variant<std::monostate, T, std::exception_ptr> state_;
};

/*
 * Specialization for operations that complete without producing any
 * value.
 */
template<>
struct CUTI_ABI async_result_t<void>
{
  async_result_t()
  : available_(false)
  , exptr_(nullptr)
  { }

  bool available() const noexcept
  {
    return available_;
  }

  std::exception_ptr exception() const noexcept
  {
    assert(this->available());
    return exptr_;
  }

  void value() const
  {
    assert(this->available());
    if(auto exptr = this->exception())
    {
      std::rethrow_exception(exptr);
    }
  }
    
  void on_success() noexcept
  {
    assert(!this->available());
    available_ = true;
  }

  template<typename Exptr>
  void on_exception(Exptr&& exptr) noexcept
  {
    assert(exptr != nullptr);
    assert(!this->available());
    available_ = true;
    exptr_ = std::forward<Exptr>(exptr);
  }

private :
  bool available_;
  std::exception_ptr exptr_;
};

/*
 * Wrapper type for a reference to an async_result_t, conforming to
 * the async continuation protocol.  Please note that the target
 * async_result_t must survive (any copies of) this wrapper type.
 */
template<typename T>
struct async_result_ref_t
{
  explicit async_result_ref_t(async_result_t<T>& target)
  : target_(target)
  { }

  template<typename... Args>
  void submit(Args&&... args) const
  {
    target_.on_success(std::forward<Args>(args)...);
  }

  template<typename Exptr>
  void fail(Exptr&& exptr) const noexcept
  {
    target_.on_exception(std::forward<Exptr>(exptr));
  }

private :
  async_result_t<T>& target_;
};

/*
 * Utility function for producing a properly typed async_result_ref_t.
 */
template<typename T>
auto async_result_ref(async_result_t<T>& target)
{
  return async_result_ref_t<T>(target);
}

} // cuti

#endif
