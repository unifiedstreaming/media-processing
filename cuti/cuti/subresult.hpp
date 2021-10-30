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

namespace cuti
{

template<typename Parent, typename T>
struct subresult_t : result_t<T>
{
  using value_t = T;

  subresult_t(Parent& parent,
              void (Parent::*on_failure)(std::exception_ptr))
  : parent_(parent)
  , on_success_(nullptr)
  , on_failure_((assert(on_failure != nullptr), on_failure))
  { }

  template<typename Child, typename... Args>
  void start_child(void (Parent::*on_success)(value_t),
                   Child& child,
                   Args&&... args)
  {
    assert(on_success != nullptr);
    on_success_ = on_success;
    child.start(std::forward<Args>(args)...);
  }
    
private :
  void do_submit(value_t value) override
  {
    assert(on_success_ != nullptr);
    (parent_.*on_success_)(std::move(value));
  }

  void do_fail(std::exception_ptr ex) override
  {
    (parent_.*on_failure_)(std::move(ex));
  }

private :
  Parent& parent_;
  void (Parent::*on_success_)(value_t);
  void (Parent::*on_failure_)(std::exception_ptr);
};

} // cuti

#endif
