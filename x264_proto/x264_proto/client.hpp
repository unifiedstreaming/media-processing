/*
 * Copyright (C) 2022-2026 CodeShop B.V.
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

#ifndef X264_PROTO_CLIENT_HPP_
#define X264_PROTO_CLIENT_HPP_

#include "linkage.h"
#include "types.hpp"

#include <cuti/endpoint.hpp>
#include <cuti/input_list.hpp>
#include <cuti/nb_client_cache.hpp>
#include <cuti/logging_context.hpp>
#include <cuti/output_list.hpp>
#include <cuti/rpc_client.hpp>
#include <cuti/throughput_checker.hpp>
#include <cuti/type_list.hpp>

#include <x26x_proto/client.hpp>
#include <x26x_proto/types.hpp>

#include <string>
#include <utility>
#include <vector>

namespace x264_proto
{

using client_t = x26x_proto::client_t<session_params_t, sample_headers_t>;

} // x264_proto

#endif
