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

#include "service.hpp"

#include <x264.h>

#include <cuti/dispatcher.hpp>
#include <cuti/add_handler.hpp>
#include <cuti/echo_handler.hpp>
#include <cuti/method_map.hpp>
#include <cuti/subtract_handler.hpp>

#include <utility>

namespace x264_es_utils
{

service_t::service_t(
  cuti::logging_context_t const& context,
  cuti::dispatcher_config_t const& dispatcher_config,
  std::vector<cuti::endpoint_t> const& endpoints)
: map_(std::make_unique<cuti::method_map_t>())
, dispatcher_(std::make_unique<cuti::dispatcher_t>(
                context, dispatcher_config))
, endpoints_()
{
  // add sample methods (for manual testing)
  map_->add_method_factory(
    "add", cuti::default_method_factory<cuti::add_handler_t>());
  map_->add_method_factory(
    "echo", cuti::default_method_factory<cuti::echo_handler_t>());
  map_->add_method_factory(
    "subtract", cuti::default_method_factory<cuti::subtract_handler_t>());

  for(auto const& endpoint : endpoints)
  {
    auto bound_endpoint = dispatcher_->add_listener(endpoint, *map_);
    endpoints_.push_back(std::move(bound_endpoint));
  }
}

void service_t::run()
{
  dispatcher_->run();
}

void service_t::stop(int sig)
{
  dispatcher_->stop(sig);
}

service_t::~service_t()
{
}

} // x264_es_utils
