/*
 * Copyright (C) 2021-2023 CodeShop B.V.
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

#ifndef CUTI_SCOPED_GUARD_HPP_
#define CUTI_SCOPED_GUARD_HPP_

#include <type_traits>
#include <utility>

namespace cuti
{

/*
 * A scoped guard object is intended to be used as a temporary
 * resource holder in scopes where exceptions are expected.  It stores
 * a function object that is called when the guard object is
 * destroyed, unless the guard object was previously dismissed or
 * moved from.
 *
 * Use make_scoped_guard(f) to instantiate a properly typed scoped
 * guard object; typically 'auto guard = make_scoped_guard(<lambda>);'
 */
template<typename F>
struct scoped_guard_t
{
  explicit scoped_guard_t(F const& f) noexcept
  : dismissed_(false)
  , f_(f)
  {
    static_assert(std::is_nothrow_copy_constructible_v<F>);
  }

  explicit scoped_guard_t(F&& f) noexcept
  : dismissed_(false)
  , f_(std::move(f))
  {
    static_assert(std::is_nothrow_move_constructible_v<F>);
  }

  scoped_guard_t(scoped_guard_t&& rhs) noexcept
  : dismissed_(rhs.dismissed_)
  , f_(std::move(rhs.f_))
  {
    static_assert(std::is_nothrow_move_constructible_v<F>);
    rhs.dismissed_ = true;
  }

  scoped_guard_t(scoped_guard_t const&) = delete;
  scoped_guard_t& operator=(scoped_guard_t const&) = delete;

  void dismiss() noexcept
  {
    dismissed_ = true;
  }

  ~scoped_guard_t()
  {
    if(!dismissed_)
    {
      f_();
    }
  }

private :
  bool dismissed_;
  F f_;
};

template<typename F>
auto make_scoped_guard(F&& f) noexcept
{
  return scoped_guard_t<std::decay_t<F>>(std::forward<F>(f));
}

} // cuti

#endif
