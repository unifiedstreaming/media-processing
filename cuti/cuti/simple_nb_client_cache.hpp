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

#ifndef CUTI_SIMPLE_NB_CLIENT_CACHE_HPP_
#define CUTI_SIMPLE_NB_CLIENT_CACHE_HPP_

#include "linkage.h"
#include "logging_context.hpp"
#include "nb_inbuf.hpp"
#include "nb_outbuf.hpp"
#include "nb_client.hpp"
#include "nb_client_cache.hpp"

#include <cstddef>
#include <list>
#include <memory>
#include <mutex>
#include <ostream>

namespace cuti
{

/*
 * A simple nb_client_cache_t implementation
 */
struct CUTI_ABI simple_nb_client_cache_t : nb_client_cache_t
{
  static std::size_t constexpr default_max_cachesize = 64;
  static std::size_t constexpr default_inbufsize =
    nb_inbuf_t::default_bufsize;
  static std::size_t constexpr default_outbufsize =
    nb_outbuf_t::default_bufsize;

  explicit simple_nb_client_cache_t(
    std::size_t max_cachesize = default_max_cachesize,
    std::size_t inbufsize = default_inbufsize,
    std::size_t outbufsize = default_outbufsize);

  std::unique_ptr<nb_client_t> obtain(
    logging_context_t const& context,
    endpoint_t const& server_address) override;

  void store(
    logging_context_t const& context,
    std::unique_ptr<nb_client_t> client) override;

  void invalidate_entries(logging_context_t const& context,
    endpoint_t const& server_address) override;

  friend CUTI_ABI
  std::ostream& operator<<(
    std::ostream& os, simple_nb_client_cache_t const& cache);

private :
  std::size_t max_cachesize_;
  std::size_t inbufsize_;
  std::size_t outbufsize_;

  std::mutex mut_;
  std::list<std::unique_ptr<nb_client_t>> clients_;
};

} // cuti

#endif
