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

#ifndef ENDPOINT_HPP_
#define ENDPOINT_HPP_

#include "socket_nifty.hpp"

#include <cstddef>
#include <string>

struct sockaddr;

namespace xes
{

using endpoint_t = sockaddr;

int endpoint_family(endpoint_t const& endpoint);
std::size_t endpoint_size(endpoint_t const& endpoint);
std::string ip_address(endpoint_t const& endpoint);
unsigned int port_number(endpoint_t const& endpoint);

} // namespace xes

#endif
