/*
 * Copyright (C) 2021-2022 CodeShop B.V.
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

#ifndef X264_SERVICE_HPP_
#define X264_SERVICE_HPP_

#include <cuti/endpoint.hpp>
#include <cuti/service.hpp>

#include <memory>
#include <vector>

namespace cuti
{

struct dispatcher_t;
struct logging_context_t;
struct selector_factory_t;
struct tcp_connection_t;

} // cuti

struct x264_service_t : cuti::service_t
{
  x264_service_t(cuti::logging_context_t const& context,
                 cuti::tcp_connection_t& control_connection,
                 cuti::selector_factory_t const& selector_factory,
                 std::vector<cuti::endpoint_t> const& endpoints);

  void run() override;

  ~x264_service_t() override;
      
private :
  std::unique_ptr<cuti::dispatcher_t> dispatcher_;
};

#endif
