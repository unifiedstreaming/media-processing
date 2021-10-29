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

#ifndef CUTI_SUBREADER_HPP_
#define CUTI_SUBREADER_HPP_

#include "result.hpp"

#include <cassert>
#include <utility>

namespace cuti
{

template<typename Parent, typename Child>
struct subreader_t
{
  using value_t = typename Child::value_t;

  template<typename... OtherArgs>
  subreader_t(Parent& parent,
              void (Parent::*on_exception)(std::exception_ptr),
              OtherArgs&&... other_args)
  : subresult_(parent, (assert(on_exception != nullptr), on_exception))
  , child_(subresult_, std::forward<OtherArgs>(other_args)...)
  { }

  subreader_t(subreader_t const&) = delete;
  subreader_t& operator=(subreader_t const&) = delete;
  
  template<typename... OtherArgs>
  void start(void (Parent::*on_value)(value_t),
             OtherArgs&&... other_args)
  {
    assert(on_value != nullptr);

    subresult_.start_child(
      on_value, child_, std::forward<OtherArgs>(other_args)...);
  }
    
private :
  struct subresult_t : result_t<value_t>
  {
    subresult_t(Parent& parent,
                void (Parent::*on_exception)(std::exception_ptr))
    : parent_(parent)
    , on_value_(nullptr)
    , on_exception_(on_exception)
    { }

    template<typename... OtherArgs>
    void start_child(void (Parent::*on_value)(value_t),
                     Child& child,
                     OtherArgs&&... other_args)
    {
      on_value_ = on_value;
      child.start(std::forward<OtherArgs>(other_args)...);
    }
    
  private :
    void do_set_value(value_t value) override
    {
      (parent_.*on_value_)(std::move(value));
    }

    void do_set_exception(std::exception_ptr ex) override
    {
      (parent_.*on_exception_)(std::move(ex));
    }

  private :
    Parent& parent_;
    void (Parent::*on_value_)(value_t);
    void (Parent::*on_exception_)(std::exception_ptr);
  };

private :
  subresult_t subresult_;
  Child child_;
};

} // cuti

#endif
