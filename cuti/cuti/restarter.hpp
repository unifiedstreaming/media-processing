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

#ifndef CUTI_RESTARTER_HPP_
#define CUTI_RESTARTER_HPP_

#include <type_traits>
#include <utility>

/*
 * A one-shot facility for capturing a function's parameters and
 * re-invoking that function at some later time.  By default, all
 * parameters are captured by value; use std::{c}ref() to capture by
 * reference.
 */

namespace cuti
{

template<typename F, typename... Args>
struct restarter_t;

template<typename F>
struct restarter_t<F>
{
  explicit restarter_t(F const& f)
  : f_(f)
  { }

  explicit restarter_t(F&& f)
  : f_(std::move(f))
  { }

  template<typename... Args>
  void operator()(Args&&... args)
  {
    f_(std::forward<Args>(args)...);
  }
    
private :
  F f_;
};

template<typename F, typename A1, typename... An>
struct restarter_t<F, A1, An...>
{
  template<typename FF, typename... AAn>
  restarter_t(FF&& f, A1 const& a1, AAn&&... an)
  : delegate_(std::forward<FF>(f), std::forward<AAn>(an)...)
  , a1_(a1)
  { }

  template<typename FF, typename... AAn>
  restarter_t(FF&& f, A1&& a1, AAn&&... an)
  : delegate_(std::forward<FF>(f), std::forward<AAn>(an)...)
  , a1_(std::move(a1))
  { }

  template<typename... Args>
  void operator()(Args&&... args)
  {
    delegate_(std::forward<Args>(args)..., std::move(a1_));
  }
    
private :
  restarter_t<F, An...> delegate_;
  A1 a1_;
};

template<typename... Args>
auto make_restarter(Args&&... args)
{
  return restarter_t<std::decay_t<Args>...>(std::forward<Args>(args)...);
}

} // cuti

#endif
