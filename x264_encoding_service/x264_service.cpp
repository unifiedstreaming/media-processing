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

#include "x264_listener.hpp"

#include <cuti/dispatcher.hpp>

x264_service_t::x264_service_t(
  cuti::logging_context_t const& context,
  cuti::event_pipe_reader_t& control_pipe,
  cuti::selector_factory_t const& selector_factory,
  std::vector<cuti::endpoint_t> const& endpoints)
: dispatcher_(std::make_unique<cuti::dispatcher_t>(
                context, control_pipe, selector_factory))
{
  for(auto const& endpoint : endpoints)
  {
    dispatcher_->add_listener(std::make_unique<x264_listener_t>(
      context, endpoint));
  }
}

void x264_service_t::run()
{
  dispatcher_->run();
}
      
x264_service_t::~x264_service_t()
{
}
