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

#ifndef CUTI_RELATIONAL_OPS_HPP_
#define CUTI_RELATIONAL_OPS_HPP_

/*
 * Mix-in base classes for generating overloads of the relational
 * operators for some user-defined type T and a set of zero or
 * more related ('Peer') types.
 */

namespace cuti
{

/*
 * Requirements for operator==(), operator!=():
 *
 * T must have a const member function named equal_to that accepts a
 * const T&.
 *
 * For each Peer in Peers, T must have a const member function named
 * equal_to that accepts a const Peer&.
 */
template<typename T, typename... Peers>
struct equality_ops_t;

template<typename T>
struct equality_ops_t<T>
{
  friend constexpr bool operator==(T const& lhs, T const& rhs)
  { return lhs.equal_to(rhs); }

  friend constexpr bool operator!=(T const& lhs, T const& rhs)
  { return !(lhs == rhs); }
};

template<typename T, typename FirstPeer, typename... OtherPeers>
struct equality_ops_t<T, FirstPeer, OtherPeers...> :
       equality_ops_t<T, OtherPeers...>
{
  friend constexpr bool operator==(T const& lhs, FirstPeer const& rhs)
  { return lhs.equal_to(rhs); }

  friend constexpr bool operator!=(T const& lhs, FirstPeer const& rhs)
  { return !(lhs == rhs); }

  friend constexpr bool operator==(FirstPeer const& lhs, T const& rhs)
  { return rhs == lhs; }

  friend constexpr bool operator!=(FirstPeer const& lhs, T const& rhs)
  { return !(lhs == rhs); }
};

/*
 * Requirements for operator<(), operator<=(), operator>(), operator>=():
 *
 * T must have a const member function named less_than that accepts a
 * const T&.
 *
 * For each Peer in Peers, T must have a const member function named
 * less_than that accepts a const Peer&.
 *
 * For each Peer in Peers, T must have a const member function named
 * greater_than that accepts a const Peer&.
 */
template<typename T, typename... Peers>
struct ordering_ops_t;

template<typename T>
struct ordering_ops_t<T>
{
  friend constexpr bool operator<(T const& lhs, T const& rhs)
  { return lhs.less_than(rhs); }

  friend constexpr bool operator>(T const& lhs, T const& rhs)
  { return rhs < lhs; }

  friend constexpr bool operator<=(T const& lhs, T const& rhs)
  { return !(lhs > rhs); }

  friend constexpr bool operator>=(T const& lhs, T const& rhs)
  { return !(lhs < rhs); }
};

template<typename T, typename FirstPeer, typename... OtherPeers>
struct ordering_ops_t<T, FirstPeer, OtherPeers...> :
       ordering_ops_t<T, OtherPeers...>
{
  friend constexpr bool operator<(T const& lhs, FirstPeer const& rhs)
  { return lhs.less_than(rhs); }

  friend constexpr bool operator>(T const& lhs, FirstPeer const& rhs)
  { return lhs.greater_than(rhs); }

  friend constexpr bool operator<=(T const& lhs, FirstPeer const& rhs)
  { return !(lhs > rhs); }

  friend constexpr bool operator>=(T const& lhs, FirstPeer const& rhs)
  { return !(lhs < rhs); }

  friend constexpr bool operator<(FirstPeer const& lhs, T const& rhs)
  { return rhs > lhs; }

  friend constexpr bool operator>(FirstPeer const& lhs, T const& rhs)
  { return rhs < lhs; }

  friend constexpr bool operator<=(FirstPeer const& lhs, T const& rhs)
  { return !(lhs > rhs); }

  friend constexpr bool operator>=(FirstPeer const& lhs, T const& rhs)
  { return !(lhs < rhs); }
};

template<typename T, typename... Peers>
struct relational_ops_t : equality_ops_t<T, Peers...>,
                          ordering_ops_t<T, Peers...>
{ };

} // cuti

#endif
