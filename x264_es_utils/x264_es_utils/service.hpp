/*
 * Copyright (C) 2021-2024 CodeShop B.V.
 *
 * This file is part of the x264_es_utils library.
 *
 * The x264_es_utils library is free software: you can redistribute it
 * and/or modify it under the terms of version 2 of the GNU General
 * Public License as published by the Free Software Foundation.
 *
 * The x264_es_utils library is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See version 2 of the GNU General Public License for more details.
 *
 * You should have received a copy of version 2 of the GNU General
 * Public License along with the x264_es_utils library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef X264_ES_UTILS_SERVICE_HPP_
#define X264_ES_UTILS_SERVICE_HPP_

#include "encoder_settings.hpp"

#include <cuti/dispatcher.hpp>
#include <cuti/endpoint.hpp>
#include <cuti/service.hpp>

#include <memory>
#include <vector>

namespace cuti
{

struct logging_context_t;
struct method_map_t;

} // cuti

namespace x264_es_utils
{

struct service_t : cuti::service_t
{
  service_t(cuti::logging_context_t const& context,
            cuti::dispatcher_config_t const& dispatcher_config,
            encoder_settings_t const& encoder_settings,
            std::vector<cuti::endpoint_t> const& endpoints);

  std::vector<cuti::endpoint_t> const& endpoints() const
  {
    return endpoints_;
  }

  void run() override;
  void stop(int sig) override;

  ~service_t() override;
      
private :
  std::unique_ptr<cuti::method_map_t> map_;
  std::unique_ptr<cuti::dispatcher_t> dispatcher_;
  std::vector<cuti::endpoint_t> endpoints_;
};

} // x264_es_utils

#endif
