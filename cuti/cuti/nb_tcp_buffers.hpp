/*
 * Copyright (C) 2021-2023 CodeShop B.V.
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

#ifndef CUTI_NB_TCP_BUFFERS_HPP_
#define CUTI_NB_TCP_BUFFERS_HPP_

#include "linkage.h"
#include "nb_inbuf.hpp"
#include "nb_outbuf.hpp"
#include "tcp_connection.hpp"

#include <cstddef>
#include <memory>
#include <utility>

namespace cuti
{

/*
 * Returns an nb_inbuf_t/nb_outbuf_t pair for reading from, and
 * writing to, a tcp connection.
 */
CUTI_ABI
std::pair<std::unique_ptr<nb_inbuf_t>, std::unique_ptr<nb_outbuf_t>>
make_nb_tcp_buffers(std::unique_ptr<tcp_connection_t> conn,
                    std::size_t inbufsize,
                    std::size_t outbufsize);

CUTI_ABI inline
std::pair<std::unique_ptr<nb_inbuf_t>, std::unique_ptr<nb_outbuf_t>>
make_nb_tcp_buffers(std::unique_ptr<tcp_connection_t> conn)
{
  return make_nb_tcp_buffers(std::move(conn),
                             nb_inbuf_t::default_bufsize,
                             nb_outbuf_t::default_bufsize);
}

} // cuti

#endif
