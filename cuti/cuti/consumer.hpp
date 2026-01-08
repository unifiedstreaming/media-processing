/*
 * Copyright (C) 2021-2026 CodeShop B.V.
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

#ifndef CUTI_CONSUMER_HPP_
#define CUTI_CONSUMER_HPP_

#include <optional>

namespace cuti
{

/*
 * Interface type consuming a stream of Ts.  The end of the stream is
 * marked by an empty optional.
 */
template<typename T>
struct consumer_t
{
  consumer_t()
  { }

  consumer_t(consumer_t const&) = delete;
  consumer_t& operator=(consumer_t const&) = delete;

  virtual void put(std::optional<T> value) = 0;

  virtual ~consumer_t()
  { }
};

} // cuti

#endif
