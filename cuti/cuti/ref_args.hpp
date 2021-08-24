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

#ifndef CUTI_REF_ARGS_HPP_
#define CUTI_REF_ARGS_HPP_

#include "linkage.h"

#include <utility>

namespace cuti
{

template<typename... Args>
struct ref_args_t;

template<>
struct CUTI_ABI ref_args_t<>
{
  template<typename FirstArg>
  auto with_first_arg(FirstArg&& first_arg) const
  {
    return ref_args_t<FirstArg>(std::forward<FirstArg>(first_arg), *this);
  }

  template<typename F>
  auto apply(F&& f) const
  {
    return this->do_apply(std::forward<F>(f));
  }

  template<typename F>
  auto apply_reversed(F&& f) const
  {
    return this->do_apply_reversed(std::forward<F>(f));
  }

private :
  template<typename... Args>
  friend struct ref_args_t;

  template<typename F, typename... PrevArgs>
  auto do_apply(F&& f, PrevArgs&&... prev_args) const
  {
    return std::forward<F>(f)(std::forward<PrevArgs>(prev_args)...);
  }
  
  template<typename F, typename... PrevArgs>
  auto do_apply_reversed(F&& f, PrevArgs&&... prev_args) const
  {
    return std::forward<F>(f)(std::forward<PrevArgs>(prev_args)...);
  }
};

template<typename Arg1, typename... ArgN>
struct ref_args_t<Arg1, ArgN...>
{
  template<typename FirstArg>
  auto with_first_arg(FirstArg&& first_arg) const
  {
    return ref_args_t<FirstArg, Arg1, ArgN...>(
      std::forward<FirstArg>(first_arg), *this);
  }

  template<typename F>
  auto apply(F&& f) const
  {
    return this->do_apply(std::forward<F>(f));
  }
    
  template<typename F>
  auto apply_reversed(F&& f) const
  {
    return this->do_apply_reversed(std::forward<F>(f));
  }
    
private :
  template<typename... Args>
  friend struct ref_args_t;

  ref_args_t(Arg1&& arg1, ref_args_t<ArgN...> const& delegate)
  : arg1_(arg1)
  , delegate_(delegate)
  { }

  template<typename F, typename... PrevArgs>
  auto do_apply(F&& f, PrevArgs&&... prev_args) const
  {
    return delegate_.do_apply(std::forward<F>(f),
      std::forward<PrevArgs>(prev_args)..., std::forward<Arg1>(arg1_));
  }
  
  template<typename F, typename... PrevArgs>
  auto do_apply_reversed(F&& f, PrevArgs&&... prev_args) const
  {
    return delegate_.do_apply_reversed(std::forward<F>(f),
      std::forward<Arg1>(arg1_), std::forward<PrevArgs>(prev_args)...);
  }
  
private :
  Arg1& arg1_;
  ref_args_t<ArgN...> delegate_;
};

inline auto ref_args()
{
  return ref_args_t<>();
}

template<typename Arg1, typename... ArgN>
auto ref_args(Arg1&& arg1, ArgN&&... argn)
{
  return ref_args(std::forward<ArgN>(argn)...).with_first_arg(
    std::forward<Arg1>(arg1));
}

} // cuti

#endif
