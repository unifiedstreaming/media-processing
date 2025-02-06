/*
 * Copyright (C) 2024-2025 CodeShop B.V.
 *
 * This file is part of the x264 service protocol library.
 *
 * The x264 service protocol library is free software: you can
 * redistribute it and/or modify it under the terms of version 2.1 of
 * the GNU Lesser General Public License as published by the Free
 * Software Foundation.
 *
 * The x264 service protocol library is distributed in the hope that
 * it will be useful, but WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See version 2.1 of the GNU Lesser General Public License
 * for more details.
 * 
 * You should have received a copy of version 2.1 of the GNU Lesser
 * General Public License along with the x264 service protocol
 * library.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef X264_PROTO_DEFAULT_ENDPOINTS_HPP_
#define X264_PROTO_DEFAULT_ENDPOINTS_HPP_

#include "linkage.h"

#include <cuti/endpoint.hpp>

#include <vector>

namespace x264_proto
{

/*
 * The list of endpoints the x264 encoding service listens on by default.
 */
X264_PROTO_ABI
std::vector<cuti::endpoint_t> default_endpoints(cuti::socket_layer_t& sockets);

} // x264_proto

#endif
