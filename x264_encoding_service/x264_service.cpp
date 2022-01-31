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

#include "x264_service.hpp"

#include <cuti/dispatcher.hpp>
#include <cuti/echo_handler.hpp>
#include <cuti/method_map.hpp>

x264_service_t::x264_service_t(
  cuti::logging_context_t const& context,
  cuti::dispatcher_config_t const& dispatcher_config,
  std::vector<cuti::endpoint_t> const& endpoints)
: map_(std::make_unique<cuti::method_map_t>())
, dispatcher_(std::make_unique<cuti::dispatcher_t>(
                context, dispatcher_config))
{
  // add echo method (for manual testing)
  map_->add_method_factory(
    "echo", cuti::default_method_factory<cuti::echo_handler_t>());

  for(auto const& endpoint : endpoints)
  {
    dispatcher_->add_listener(endpoint, *map_);
  }
}

void x264_service_t::run()
{
  dispatcher_->run();
}
      
void x264_service_t::stop(int sig)
{
  dispatcher_->stop(sig);
}
      
x264_service_t::~x264_service_t()
{
}
