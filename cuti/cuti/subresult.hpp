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

#ifndef CUTI_SUBRESULT_HPP_
#define CUTI_SUBRESULT_HPP_

#include "result.hpp"

#include <cassert>
#include <utility>

namespace cuti
{

/*
 * Helper to deal with subresult_t<Parent, void>
 */
template<typename Parent, typename T>
struct subresult_helper_t
{
  using on_success_t = void (Parent::*)(T);

  template<typename TT>
  static void report_success(
    Parent& parent, on_success_t on_success, TT&& value)
  {
    (parent.*on_success)(std::forward<TT>(value));
  }
};
    
template<typename Parent>
struct subresult_helper_t<Parent, void>
{
  using on_success_t = void (Parent::*)();

  template<typename TT>
  static void report_success(
    Parent& parent, on_success_t on_success, TT&& /*ignored */)
  {
    (parent.*on_success)();
  }
};

/*
 * Result type for an asynchronous suboperation.
 */
template<typename Parent, typename T>
struct subresult_t : result_t<T>
{
  using submit_arg_t = typename result_t<T>::submit_arg_t;
  using on_success_t = typename subresult_helper_t<Parent, T>::on_success_t;
  using on_failure_t = void (Parent::*)(std::exception_ptr);

  subresult_t(Parent& parent, on_failure_t on_failure)
  : parent_(parent)
  , on_success_(nullptr)
  , on_failure_((assert(on_failure != nullptr), on_failure))
  { }

  template<typename Child, typename... Args>
  void start_child(on_success_t on_success, Child& child, Args&&... args)
  {
    assert(on_success != nullptr);
    on_success_ = on_success;
    child.start(std::forward<Args>(args)...);
  }
    
private :
  void do_submit(submit_arg_t value) override
  {
    assert(on_success_ != nullptr);
    subresult_helper_t<Parent, T>::report_success(
      parent_, on_success_, std::move(value));
  }

  void do_fail(std::exception_ptr ex) override
  {
    (parent_.*on_failure_)(std::move(ex));
  }

private :
  Parent& parent_;
  on_success_t on_success_;
  on_failure_t on_failure_;
};

} // cuti

#endif
