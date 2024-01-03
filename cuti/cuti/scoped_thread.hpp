/*
 * Copyright (C) 2021-2024 CodeShop B.V.
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

#ifndef CUTI_SCOPED_THREAD_HPP_
#define CUTI_SCOPED_THREAD_HPP_

#include "linkage.h"

#include <thread>
#include <type_traits>
#include <utility>

namespace cuti
{

/*
 * Wrapper for std::thread automatically joining the thread upon
 * destruction.
 */
struct CUTI_ABI scoped_thread_t
{
  template<typename F, typename = std::enable_if_t<
    !std::is_convertible_v<std::decay_t<F>*, scoped_thread_t const*>>>
  explicit scoped_thread_t(F&& f)
  : thread_(std::forward<F>(f))
  { }

  scoped_thread_t(scoped_thread_t const&) = delete;
  scoped_thread_t& operator=(scoped_thread_t const&) = delete;

  ~scoped_thread_t();

private :
  std::thread thread_;
};

} // namespace cuti

#endif
