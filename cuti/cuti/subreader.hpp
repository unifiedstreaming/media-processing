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

#include "subresult.hpp"

#include <cassert>
#include <utility>

namespace cuti
{

template<typename Parent, typename Child>
struct subreader_t
{
  using value_t = typename Child::value_t;
  using on_success_t = typename subresult_t<Parent, value_t>::on_success_t;
  using on_failure_t = typename subresult_t<Parent, value_t>::on_failure_t;
  
  template<typename... ChildArgs>
  subreader_t(Parent& parent,
              on_failure_t on_failure,
              ChildArgs&&... child_args)
  : subresult_(parent, on_failure)
  , child_(subresult_, std::forward<ChildArgs>(child_args)...)
  { }

  subreader_t(subreader_t const&) = delete;
  subreader_t& operator=(subreader_t const&) = delete;
  
  template<typename... Args>
  void start(on_success_t on_success, Args&&... args)
  {
    subresult_.start_child(
      on_success, child_, std::forward<Args>(args)...);
  }
    
private :
  subresult_t<Parent, value_t> subresult_;
  Child child_;
};

} // cuti

#endif