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

#ifndef CUTI_NB_CLIENT_HPP_
#define CUTI_NB_CLIENT_HPP_

#include "endpoint.hpp"
#include "linkage.h"
#include "nb_inbuf.hpp"
#include "nb_outbuf.hpp"

#include <cstddef>
#include <ostream>
#include <memory>

namespace cuti
{

struct socket_layer_t;

/*
 * A pair of non-blocking buffers representing the client side of a
 * TCP connection.
 */
struct CUTI_ABI nb_client_t
{
  explicit nb_client_t(socket_layer_t& sockets,
                       endpoint_t server_address,
                       std::size_t inbufsize = nb_inbuf_t::default_bufsize,
                       std::size_t outbufsize = nb_outbuf_t::default_bufsize);

  nb_client_t(nb_client_t const&) = delete;
  nb_client_t& operator=(nb_client_t const&) = delete;

  endpoint_t const& server_address() const
  { return server_address_; }

  nb_inbuf_t& nb_inbuf()
  { return *nb_inbuf_; }

  nb_inbuf_t const& nb_inbuf() const
  { return *nb_inbuf_; }

  nb_outbuf_t& nb_outbuf()
  { return *nb_outbuf_; }

  nb_outbuf_t const& nb_outbuf() const
  { return *nb_outbuf_; }

  friend std::ostream& operator<<(std::ostream& os, nb_client_t const& client)
  { return os << *client.nb_inbuf_; }

private :
  endpoint_t server_address_;
  std::unique_ptr<nb_inbuf_t> nb_inbuf_;
  std::unique_ptr<nb_outbuf_t> nb_outbuf_;
};

} // cuti

#endif
