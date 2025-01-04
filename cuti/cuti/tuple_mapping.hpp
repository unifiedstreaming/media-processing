/*
 * Copyright (C) 2021-2025 CodeShop B.V.
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

#ifndef CUTI_TUPLE_MAPPING_HPP_
#define CUTI_TUPLE_MAPPING_HPP_

#include <tuple>
#include <utility> // for std::pair

namespace cuti
{

/*
 * Specialize this class template to enable cuti serialization for a
 * user-defined non-enum type T.
 *
 * The specialization must contain:
 *
 * - a type named tuple_t that defines a tuple-like type (std::tuple,
 * std::pair, std::array) containing the (field) values to be
 * serialized.
 *
 * - a static member function named to_tuple() taking a T (by value,
 * const reference or rvalue reference) and returns a tuple_t
 * instance.
 *
 * - a static member function named from_tuple() taking a tuple_t (by
 * value, const reference, or rvalue reference) and returns a T.  This
 * function may report errors by throwing something derived from
 * std::exception.
 *
 * Please see remote_error.hpp for an example specialization; C++
 * allows us to define a specialization of cuti::tuple_mapping_t in
 * either the cuti namespace or the global namespace, but not in a
 * user namespace.
 */
template<typename T>
struct tuple_mapping_t;

} // cuti

#endif
