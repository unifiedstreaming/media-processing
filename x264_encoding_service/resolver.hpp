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

#ifndef XES_RESOLVER_HPP_
#define XES_RESOLVER_HPP_

#include "endpoint.hpp"
#include "socket_nifty.hpp"

#include <string>
#include <vector>

namespace xes
{

/*
 * Endpoint factory functions
 */
using endpoints_t = std::vector<endpoint_t>;
unsigned int const any_port = 0;

// Returns an endpoint for an IP address and port number
endpoint_t resolve_ip(char const* ip, unsigned int port);
endpoint_t resolve_ip(std::string const& ip, unsigned int port);

// Returns endpoints for a host name and port number
endpoints_t resolve_host(char const* host, unsigned int port);
endpoints_t resolve_host(std::string const& host, unsigned int port);

// Returns endpoints for binding to local interfaces
endpoints_t local_interfaces(unsigned int port);

// Returns endpoints for binding to all interfaces
endpoints_t all_interfaces(unsigned int port);

} // xes

#endif
