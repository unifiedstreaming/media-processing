/*
 * Copyright (C) 2021-2025 CodeShop B.V.
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

#ifndef CUTI_RESOLVER_HPP_
#define CUTI_RESOLVER_HPP_

#include "endpoint.hpp"
#include "linkage.h"
#include "socket_nifty.hpp"

#include <cinttypes>
#include <limits>
#include <string>
#include <vector>

namespace cuti
{

/*
 * Endpoint factory functions
 */
using endpoints_t = std::vector<endpoint_t>;
unsigned int constexpr any_port = 0;
unsigned int constexpr max_port = std::numeric_limits<uint16_t>::max();

// Returns an endpoint for an IP address and port number
CUTI_ABI endpoint_t resolve_ip(char const* ip, unsigned int port);
CUTI_ABI endpoint_t resolve_ip(std::string const& ip, unsigned int port);

// Returns endpoints for a host name and port number
CUTI_ABI endpoints_t resolve_host(char const* host, unsigned int port);
CUTI_ABI endpoints_t resolve_host(std::string const& host, unsigned int port);

// Returns endpoints for binding to local interfaces
CUTI_ABI endpoints_t local_interfaces(unsigned int port);

// Returns endpoints for binding to all interfaces
CUTI_ABI endpoints_t all_interfaces(unsigned int port);

} // cuti

#endif
