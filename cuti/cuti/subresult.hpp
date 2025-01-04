/*
 * Copyright (C) 2021-2025 CodeShop B.V.
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
#include "stack_marker.hpp"

#include <cassert>
#include <utility>

namespace cuti
{

enum class failure_mode_t
{
  forward_upwards,
  handle_in_parent
};

template<typename Parent, failure_mode_t Mode>
struct failure_handler_t;

template<typename Parent>
struct failure_handler_t<Parent, failure_mode_t::forward_upwards>
{
  using handler_type = any_result_t&;

  explicit failure_handler_t(any_result_t& parents_result)
  : parents_result_(parents_result)
  { }

  void operator()(stack_marker_t& base_marker,
                  Parent& /* ignored */,
                  std::exception_ptr ex) const
  {
    parents_result_.fail(base_marker, std::move(ex));
  }

private :
  any_result_t& parents_result_;
};

template<typename Parent>
struct failure_handler_t<Parent, failure_mode_t::handle_in_parent>
{
  using handler_type = void (Parent::*)(stack_marker_t&, std::exception_ptr);

  explicit failure_handler_t(
    void (Parent::*handler)(stack_marker_t&, std::exception_ptr)
  )
  : handler_((assert(handler != nullptr), handler))
  { }

  void operator()(stack_marker_t& base_marker,
                  Parent& parent,
                  std::exception_ptr ex) const
  {
    (parent.*handler_)(base_marker, std::move(ex));
  }

private :
  void (Parent::*handler_)(stack_marker_t&, std::exception_ptr);
};

/*
 * Helper to deal with subresult_t<Parent, void>
 */
template<typename Parent, typename T>
struct subresult_reporter_t
{
  using on_success_t = void (Parent::*)(stack_marker_t&, T);

  template<typename TT>
  static void report_success(
    stack_marker_t& base_marker,
    Parent& parent,
    on_success_t on_success,
    TT&& value)
  {
    (parent.*on_success)(base_marker, std::forward<TT>(value));
  }
};
    
template<typename Parent>
struct subresult_reporter_t<Parent, void>
{
  using on_success_t = void (Parent::*)(stack_marker_t&);

  template<typename TT>
  static void report_success(
    stack_marker_t& base_marker,
    Parent& parent,
    on_success_t on_success,
    TT&& /*ignored */)
  {
    (parent.*on_success)(base_marker);
  }
};

/*
 * Result type for an asynchronous suboperation.
 */
template<typename Parent, typename T, failure_mode_t Mode>
struct subresult_t : result_t<T>
{
  using submit_arg_t = typename result_t<T>::submit_arg_t;
  using on_success_t = typename subresult_reporter_t<Parent, T>::on_success_t;
  using on_failure_t = typename failure_handler_t<Parent, Mode>::handler_type;

  subresult_t(Parent& parent, on_failure_t on_failure)
  : parent_(parent)
  , on_success_(nullptr)
  , failure_handler_(on_failure)
  { }

  template<typename Child, typename... Args>
  void start_child(
    stack_marker_t& base_marker,
    on_success_t on_success,
    Child& child,
    Args&&... args)
  {
    assert(on_success != nullptr);
    on_success_ = on_success;
    child.start(base_marker, std::forward<Args>(args)...);
  }
    
private :
  void do_submit(stack_marker_t& base_marker, submit_arg_t value) override
  {
    assert(on_success_ != nullptr);
    subresult_reporter_t<Parent, T>::report_success(
      base_marker, parent_, on_success_, std::move(value));
  }

  void do_fail(stack_marker_t& base_marker, std::exception_ptr ex) override
  {
    failure_handler_(base_marker, parent_, std::move(ex));
  }

private :
  Parent& parent_;
  on_success_t on_success_;
  failure_handler_t<Parent, Mode> failure_handler_;
};

} // cuti

#endif
