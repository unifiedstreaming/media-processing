/*
 * Copyright (C) 2026 CodeShop B.V.
 *
 * This file is part of the x26x_es_utils library.
 *
 * The x26x_es_utils library is free software: you can redistribute it
 * and/or modify it under the terms of version 2 of the GNU General
 * Public License as published by the Free Software Foundation.
 *
 * The x26x_es_utils library is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See version 2 of the GNU General Public License for more details.
 *
 * You should have received a copy of version 2 of the GNU General
 * Public License along with the x26x_es_utils library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef X26X_ES_UTILS_SERVICE_HPP_
#define X26X_ES_UTILS_SERVICE_HPP_

#include "encode_handler.hpp"

#include <cuti/add_handler.hpp>
#include <cuti/dispatcher.hpp>
#include <cuti/dispatcher.hpp>
#include <cuti/echo_handler.hpp>
#include <cuti/endpoint.hpp>
#include <cuti/method.hpp>
#include <cuti/method_map.hpp>
#include <cuti/service.hpp>
#include <cuti/subtract_handler.hpp>

#include <memory>
#include <vector>

namespace x26x_es_utils
{

template<typename EncoderSettings, typename EncodingSession,
         typename SessionParams, typename SampleHeaders>
struct service_t : cuti::service_t
{
  service_t(cuti::logging_context_t const& context,
            cuti::socket_layer_t& sockets,
            cuti::dispatcher_config_t const& dispatcher_config,
            EncoderSettings const& encoder_settings,
            std::vector<cuti::endpoint_t> const& endpoints)
  : map_(std::make_unique<cuti::method_map_t>())
  , dispatcher_(std::make_unique<cuti::dispatcher_t>(
                  context, sockets, dispatcher_config))
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
      return cuti::make_method<encode_handler_t<EncoderSettings,
        EncodingSession, SessionParams, SampleHeaders>>(result, context, inbuf,
        outbuf, encoder_settings);
    };
    map_->add_method_factory(
      "encode", std::move(encode_method_factory));

    for(auto const& endpoint : endpoints)
    {
      auto bound_endpoint = dispatcher_->add_listener(endpoint, *map_);
      endpoints_.push_back(std::move(bound_endpoint));
    }
  }

  std::vector<cuti::endpoint_t> const& endpoints() const
  {
    return endpoints_;
  }

  void run() override
  {
    dispatcher_->run();
  }

  void stop(int sig) override
  {
    dispatcher_->stop(sig);
  }

  ~service_t() override
  { }
      
private :
  std::unique_ptr<cuti::method_map_t> map_;
  std::unique_ptr<cuti::dispatcher_t> dispatcher_;
  std::vector<cuti::endpoint_t> endpoints_;
};

} // x26x_es_utils

#endif
