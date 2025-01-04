/*
 * Copyright (C) 2024-2025 CodeShop B.V.
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

#ifndef CUTI_NB_CLIENT_CACHE_HPP_
#define CUTI_NB_CLIENT_CACHE_HPP_

#include "endpoint.hpp"
#include "linkage.h"
#include "logging_context.hpp"
#include "nb_client.hpp"

#include <memory>

namespace cuti
{

/*
 * Abstract interface for caching nb_client objects
 */
struct CUTI_ABI nb_client_cache_t
{
  nb_client_cache_t()
  { }

  nb_client_cache_t(nb_client_cache_t const&) = delete;
  nb_client_cache_t& operator=(nb_client_cache_t const&) = delete;
  
  /*
   * Returns a (possibly previously used) nb_client instance connected
   * to server_address.
   */
  virtual std::unique_ptr<nb_client_t> obtain(
    logging_context_t const& context,
    endpoint_t const& server_address) = 0;

  /*
   * Caches an nb_client instance for possible later reuse.
   */
  virtual void store(
    logging_context_t const& context,
    std::unique_ptr<nb_client_t> client) = 0;

  /*
   * Removes all stored cache entries for a specific server address.
   */
  virtual void invalidate_entries(logging_context_t const& context,
    endpoint_t const& server_address) = 0;

  virtual ~nb_client_cache_t();
};

} // cuti

#endif
