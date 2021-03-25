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

#ifndef ENDPOINT_STORAGE_HPP_
#define ENDPOINT_STORAGE_HPP_

#include "endpoint.hpp"
#include "socket_nifty.hpp"

#include <memory>

namespace xes
{

/*
 * Storage for an endpoint of a specific address family
 */
struct endpoint_storage_t
{
  struct impl_t;

  // Zero-initializes the endpoint (but sets the address family).
  explicit endpoint_storage_t(int family);

  endpoint_storage_t(endpoint_storage_t const&) = delete;
  endpoint_storage_t& operator=(endpoint_storage_t const&) = delete;

  endpoint_t* get();
  endpoint_t const* get() const;

  endpoint_t* operator->()
  { return get(); }

  endpoint_t const* operator->() const
  { return get(); }

  endpoint_t& operator*()
  { return *get(); }

  endpoint_t const& operator*() const
  { return *get(); }

  ~endpoint_storage_t();

private :
  std::unique_ptr<impl_t> impl_;
};
  
} // xes

#endif
