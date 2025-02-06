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

#include "nb_client.hpp"
#include "nb_tcp_buffers.hpp"

#include <tuple>

namespace cuti
{

nb_client_t::nb_client_t(socket_layer_t& sockets,
                         endpoint_t server_address,
                         std::size_t inbufsize,
                         std::size_t outbufsize)
: server_address_(std::move(server_address))
, nb_inbuf_()
, nb_outbuf_()
{
  std::tie(nb_inbuf_, nb_outbuf_) = make_nb_tcp_buffers(
    std::make_unique<tcp_connection_t>(sockets, server_address_),
    inbufsize,
    outbufsize
  );
}

} // cuti
