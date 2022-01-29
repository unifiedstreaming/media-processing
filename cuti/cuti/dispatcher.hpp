/*
 * Copyright (C) 2021-2022 CodeShop B.V.
 *
 * This file is part of the cuti library.
 *
 * The cuti library is free software: you can redistribute it and/or
 * modify it under the terms of version 2.1 of the GNU Lesser General
 * Public License as published by the Free Software Foundation.
 *
 * The cuti library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See version
 * 2.1 of the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of version 2.1 of the GNU Lesser
 * General Public License along with the cuti library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef CUTI_DISPATCHER_HPP_
#define CUTI_DISPATCHER_HPP_

#include "chrono_types.hpp"
#include "endpoint.hpp"
#include "linkage.h"
#include "selector_factory.hpp"

#include <cstddef>
#include <memory>

namespace cuti
{

struct event_pipe_reader_t;
struct logging_context_t;
struct method_map_t;

struct CUTI_ABI dispatcher_config_t
{
  static selector_factory_t default_selector_factory()
  { return selector_factory_t(); }

  static std::size_t default_min_bytes_per_tick()
  { return 512; }

  static unsigned int default_low_ticks_limit()
  { return 120; }

  static duration_t default_tick_length()
  { return seconds_t(1); }
  
  dispatcher_config_t()
  : selector_factory_(default_selector_factory())
  , min_bytes_per_tick_(default_min_bytes_per_tick())
  , low_ticks_limit_(default_low_ticks_limit())
  , tick_length_(default_tick_length())
  { }

  selector_factory_t selector_factory_;
  std::size_t min_bytes_per_tick_;
  unsigned int low_ticks_limit_;
  duration_t tick_length_;
};

struct CUTI_ABI dispatcher_t
{
  dispatcher_t(logging_context_t const& logging_context,
               event_pipe_reader_t& control,
               dispatcher_config_t config);

  dispatcher_t(dispatcher_t const&) = delete;
  dispatcher_t& operator=(dispatcher_t const&) = delete;

  endpoint_t add_listener(endpoint_t const& endpoint, method_map_t const& map);
  
  void run();

  ~dispatcher_t();

private :
  struct impl_t;
  std::unique_ptr<impl_t> impl_;
};

} // cuti

#endif
