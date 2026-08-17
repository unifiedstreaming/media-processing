/*
 * Copyright (C) 2024-2026 CodeShop B.V.
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

#include "chrono_types.hpp"
#include "linkage.h"
#include "logging_context.hpp"
#include "nb_inbuf.hpp"
#include "nb_outbuf.hpp"
#include "nb_client.hpp"
#include "nb_client_cache.hpp"

#include <cassert>
#include <cstddef>
#include <list>
#include <memory>
#include <mutex>
#include <ostream>
#include <utility>

namespace cuti
{

struct socket_layer_t;

/*
 * A simple nb_client_cache_t implementation
 */
struct CUTI_ABI simple_nb_client_cache_t : nb_client_cache_t
{
  struct CUTI_ABI settings_t
  {
    static std::size_t constexpr default_max_cachesize = 64;
    static std::size_t constexpr default_inbufsize =
      nb_inbuf_t::default_bufsize;
    static std::size_t constexpr default_outbufsize =
      nb_outbuf_t::default_bufsize;
    static duration_t constexpr default_max_age = seconds_t{118};

    settings_t()
    : max_cachesize_(default_max_cachesize)
    , inbufsize_(default_inbufsize)
    , outbufsize_(default_outbufsize)
    , max_age_(default_max_age)
    { }

    std::size_t max_cachesize_;
    std::size_t inbufsize_;
    std::size_t outbufsize_;
    duration_t max_age_;
  };

  explicit simple_nb_client_cache_t(
    socket_layer_t& sockets,
    settings_t settings = settings_t{}
  );

  socket_layer_t& socket_layer() override;

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
  struct CUTI_ABI entry_t
  {
    explicit entry_t(std::unique_ptr<nb_client_t> client)
    : timestamp_(cuti_clock_t::now())
    , client_((assert(client != nullptr), std::move(client)))
    { }

    time_point_t const timestamp_;
    std::unique_ptr<nb_client_t> client_;
  };

  using entry_list_t = std::list<entry_t>;
    
  socket_layer_t& sockets_;
  settings_t const settings_;

  std::mutex mut_;
  entry_list_t entries_; // in reverse timestamp order (highest first)
};

} // cuti

#endif
