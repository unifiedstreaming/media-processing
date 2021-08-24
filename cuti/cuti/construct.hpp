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

#ifndef CUTI_CONSTRUCT_HPP_
#define CUTI_CONSTRUCT_HPP_

#include <utility>

namespace cuti
{

template<typename T>
struct construct_t
{
  template<typename... Args>
  auto operator()(Args&&... args) const
  {
    return T{std::forward<Args>(args)...};
  }
};

template<typename T>
auto constexpr construct = construct_t<T>{};

} // namespace cuti

#endif
