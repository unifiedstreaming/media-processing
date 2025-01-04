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

#ifndef CUTI_RESULT_HPP_
#define CUTI_RESULT_HPP_

#include "linkage.h"
#include "stack_marker.hpp"

#include <cassert>
#include <exception>
#include <utility>

namespace cuti
{

/*
 * Base interface for results of any type.
 */
struct any_result_t
{
  any_result_t()
  { }

  any_result_t(any_result_t const&) = delete;
  any_result_t& operator=(any_result_t const&) = delete;
  
  void fail(stack_marker_t& base_marker, std::exception_ptr ex)
  {
    assert(ex != nullptr);
    this->do_fail(base_marker, std::move(ex));
  }

  virtual ~any_result_t()
  { }

private :
  virtual void do_fail(stack_marker_t& base_marker, std::exception_ptr ex) = 0;
};

/*
 * Strawman type for reporting no meaningful value.
 */
struct CUTI_ABI no_value_t { };

/*
 * Helper to deal with result_t<void>.
 */
template<typename T>
struct result_helper_t
{
  using submit_arg_t = T;
};

template<>
struct result_helper_t<void>
{
  using submit_arg_t = no_value_t;
};

/*
 * Interface for reporting the result of an asynchronous operation.
 */
template<typename T>
struct result_t : any_result_t
{
  using submit_arg_t = typename result_helper_t<T>::submit_arg_t;

  void submit(stack_marker_t& base_marker, submit_arg_t value = no_value_t{})
  {
    this->do_submit(base_marker, std::move(value));
  }

private :
  virtual void do_submit(stack_marker_t& base_marker, submit_arg_t value) = 0;
};

} // cuti

#endif
