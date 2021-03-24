/*
 * Copyright (C) 2021 CodeShop B.V.
 *
 * This file is part of the x264_encoding_service.
 *
 * The x264_encoding_service is free software: you can redistribute it
 * and/or modify it under the terms of version 2 of the GNU General
 * Public License as published by the Free Software Foundation.
 *
 * The x264_encoding_service is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See version 2 of the GNU General Public License for more details.
 *
 * You should have received a copy of version 2 of the GNU General
 * Public License along with the x264_encoding_service.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef ENDPOINT_LIST_HHP_
#define ENDPOINT_LIST_HPP_

#include "endpoint.hpp"
#include "socket_nifty.hpp"

#include <cassert>
#include <cstddef>
#include <iterator>
#include <memory>
#include <string>

struct addrinfo;

namespace xes
{

struct endpoint_list_iterator_t
{
  using iterator_category = std::forward_iterator_tag;
  using value_type = endpoint_t;
  using difference_type = std::ptrdiff_t;
  using pointer = endpoint_t const*;
  using reference = endpoint_t const&;
  
  endpoint_list_iterator_t()
  : node_(nullptr)
  { }

  explicit endpoint_list_iterator_t(addrinfo const* node)
  : node_(node)
  { }

  bool operator==(endpoint_list_iterator_t const& rhs) const
  { return node_ == rhs.node_; }

  bool operator!=(endpoint_list_iterator_t const& rhs) const
  { return node_ != rhs.node_; }

  endpoint_list_iterator_t& operator++();

  endpoint_list_iterator_t operator++(int)
  {
    endpoint_list_iterator_t result = *this;
    ++(*this);
    return result;
  }

  endpoint_t const& operator*() const;

private :
  addrinfo const* node_;
};

struct local_interfaces_t { };
extern local_interfaces_t const local_interfaces;

struct any_interface_t { };
extern any_interface_t const any_interface;

/*
 * DNS resolver interface for TCP
 */
struct endpoint_list_t
{
  using iterator = endpoint_list_iterator_t;
  using const_iterator = iterator;
  using value_type = iterator::value_type;
  using reference = iterator::reference;
  using const_reference = reference;
  using difference_type = iterator::difference_type;
  using size_type = std::size_t;

  endpoint_list_t()
  : head_()
  { }

  explicit endpoint_list_t(local_interfaces_t const&, unsigned int port = 0);
  explicit endpoint_list_t(any_interface_t const&, unsigned int port = 0);
  explicit endpoint_list_t(char const* host, unsigned int port = 0);
  explicit endpoint_list_t(std::string const& host, unsigned int port = 0);

  bool empty() const
  { return head_ == nullptr; }

  const_iterator begin() const
  { return const_iterator(head_.get()); }

  const_iterator cbegin() const
  { return begin(); }

  const_iterator end() const
  { return const_iterator(); }

  const_iterator cend() const
  { return end(); }

  const_reference front() const
  {
    assert(!empty());
    return *begin();
  }
  
private :
  std::shared_ptr<addrinfo const> head_;
};
  
} // xes

#endif
