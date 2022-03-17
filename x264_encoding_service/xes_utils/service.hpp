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

#ifndef XES_UTILS_SERVICE_HPP_
#define XES_UTILS_SERVICE_HPP_

#include <cuti/dispatcher.hpp>
#include <cuti/endpoint.hpp>
#include <cuti/service.hpp>

#include <memory>
#include <vector>

namespace cuti
{

struct logging_context_t;
struct method_map_t;
struct tcp_connection_t;

} // cuti

namespace xes_utils
{

struct service_t : cuti::service_t
{
  service_t(cuti::logging_context_t const& context,
            cuti::dispatcher_config_t const& dispatcher_config,
            std::vector<cuti::endpoint_t> const& endpoints);

  void run() override;
  void stop(int sig) override;

  ~service_t() override;
      
private :
  std::unique_ptr<cuti::method_map_t> map_;
  std::unique_ptr<cuti::dispatcher_t> dispatcher_;
};

} // xes_utils

#endif
