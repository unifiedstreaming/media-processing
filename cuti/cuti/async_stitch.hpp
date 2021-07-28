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

#ifndef CUTI_ASYNC_STITCH_HPP_
#define CUTI_ASYNC_STITCH_HPP_

#include <type_traits>
#include <utility>

namespace cuti
{

/*
 * async_link_t models the conversion of an async function object and
 * a continuation into an object that is *itself* a continuation
 * implementing both submit() and fail().
 */
template<typename F, typename Cont>
struct async_link_t
{
  template<typename FF, typename CCont>
  async_link_t(FF&& f, CCont&& cont)
  : f_(std::forward<FF>(f))
  , cont_(std::forward<CCont>(cont))
  { }

  template<typename... Args>
  void submit(Args&&... args) const
  {
    f_(cont_, std::forward<Args>(args)...);
  }

  template<typename Exptr>
  void fail(Exptr&& exptr) const
  {
    cont_.fail(std::forward<Exptr>(exptr));
  }
    
private :
  F f_;
  Cont cont_;
};

/*
 * Convenience function for generating an async_link_t.
 */
template<typename F, typename Cont>
auto async_link(F&& f, Cont&& cont)
{
  return async_link_t<std::decay_t<F>, std::decay_t<Cont>>(
    std::forward<F>(f), std::forward<Cont>(cont));
}

/*
 * async_stitch_t models the stitching of two async function objects
 * into a compound async function object.  The second function object
 * serves as a continuation of the first function object, and uses the
 * continuation that is passed to async_stich_t's function call
 * operator.
 */
template<typename F1, typename F2>
struct async_stitch_t
{
  template<typename FF1, typename FF2>
  constexpr async_stitch_t(FF1&& f1, FF2&& f2)
  : f1_(std::forward<FF1>(f1))
  , f2_(std::forward<FF2>(f2))
  { }

  template<typename Cont, typename... Args>
  void operator()(Cont&& cont, Args&&... args) const
  {
    auto link = async_link(f2_, std::forward<Cont>(cont));
    f1_(link, std::forward<Args>(args)...);
  }
    
private :
  F1 f1_;
  F2 f2_;
};

/*
 * Convenience functions for generating stitched async function
 * objects.
 */
template<typename F1, typename F2>
constexpr auto async_stitch(F1&& f1, F2&& f2)
{
  return async_stitch_t<std::decay_t<F1>, std::decay_t<F2>>(
    std::forward<F1>(f1), std::forward<F2>(f2));
}

template<typename F1, typename F2, typename... Fn>
constexpr auto async_stitch(F1&& f1, F2&& f2, Fn&&... fn)
{
  return async_stitch(std::forward<F1>(f1),
           async_stitch(std::forward<F2>(f2), std::forward<Fn>(fn)...));
}

} // cuti

#endif
