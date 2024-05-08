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

#include "encode_handler.hpp"
#include "encoder_settings.hpp"

#include <x264.h>

#include <cuti/dispatcher.hpp>
#include <cuti/add_handler.hpp>
#include <cuti/echo_handler.hpp>
#include <cuti/method.hpp>
#include <cuti/method_map.hpp>
#include <cuti/subtract_handler.hpp>

#include <utility>

namespace x264_es_utils
{

service_t::service_t(
  cuti::logging_context_t const& context,
  cuti::dispatcher_config_t const& dispatcher_config,
  encoder_settings_t const& encoder_settings,
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

  // add encode method
  auto encode_method_factory = [encoder_settings](
    cuti::result_t<void>& result,
    cuti::logging_context_t const& context,
    cuti::bound_inbuf_t& inbuf,
    cuti::bound_outbuf_t& outbuf)
  {
    return cuti::make_method<encode_handler_t>(
      result, context, inbuf, outbuf, encoder_settings);
  };
  map_->add_method_factory(
    "encode", std::move(encode_method_factory));
   
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
