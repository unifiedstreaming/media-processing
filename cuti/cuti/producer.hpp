/*
 * Copyright (C) 2021-2022 CodeShop B.V.
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

#ifndef CUTI_PRODUCER_HPP_
#define CUTI_PRODUCER_HPP_

#include <optional>

namespace cuti
{

/*
 * Interface type producing a stream of Ts.  The end of the stream is
 * marked by an empty optional.
 */
template<typename T>
struct producer_t
{
  producer_t()
  { }

  producer_t(producer_t const&) = delete;
  producer_t& operator=(producer_t const&) = delete;

  virtual std::optional<T> get() = 0;

  virtual ~producer_t()
  { }
};

} // cuti

#endif
