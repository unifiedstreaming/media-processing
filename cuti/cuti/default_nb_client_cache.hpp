/*
 * Copyright (C) 2026 CodeShop B.V.
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

#ifndef CUTI_DEFAULT_NB_CLIENT_CACHE_HPP_
#define CUTI_DEFAULT_NB_CLIENT_CACHE_HPP_

#include "endpoint.hpp"
#include "linkage.h"
#include "logging_context.hpp"
#include "nb_client.hpp"
#include "nb_client_cache.hpp"
#include "nb_inbuf.hpp"
#include "nb_outbuf.hpp"
#include "resource_cache.hpp"

#include <cstddef>
#include <memory>
#include <ostream>

namespace cuti
{

struct socket_layer_t;

/*
 * An nb_client_cache implementation that uses the resource_cache template
 */
struct CUTI_ABI default_nb_client_cache_t : nb_client_cache_t
{
  struct CUTI_ABI settings_t
  {
    static std::size_t constexpr default_inbufsize =
      nb_inbuf_t::default_bufsize;
    static std::size_t constexpr default_outbufsize =
      nb_outbuf_t::default_bufsize;

    static resource_cache_settings_t
    constexpr default_resource_cache_settings =
      resource_cache_settings_t{};

    constexpr settings_t()
    : inbufsize_(default_inbufsize)
    , outbufsize_(default_outbufsize)
    , resource_cache_settings_(default_resource_cache_settings)
    { }

    std::size_t inbufsize_;
    std::size_t outbufsize_;
    resource_cache_settings_t resource_cache_settings_;
  };

  explicit default_nb_client_cache_t(
    socket_layer_t& sockets,
    settings_t const& settings = settings_t{}
  );

  std::unique_ptr<nb_client_t> obtain(
    logging_context_t const& context,
    endpoint_t const& server_address) override;

  void store(
    logging_context_t const& context,
    std::unique_ptr<nb_client_t> client) override;

  void invalidate_entries(
    logging_context_t const& context,
    endpoint_t const& server_address) override;

  ~default_nb_client_cache_t() override;

  friend CUTI_ABI
  std::ostream& operator<<(
    std::ostream& os, default_nb_client_cache_t const& cache)
  { return os << "default_nb_client_cache@" << &cache; }

private :
  resource_cache_t<endpoint_t, nb_client_t> resource_cache_;
};

} // cuti

#endif
