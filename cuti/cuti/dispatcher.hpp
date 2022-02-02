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
#include "nb_inbuf.hpp"
#include "selector_factory.hpp"
#include "throughput_checker.hpp"

#include <cstddef>
#include <memory>

namespace cuti
{

struct logging_context_t;
struct method_map_t;

struct CUTI_ABI dispatcher_config_t
{
  static selector_factory_t default_selector_factory()
  { return selector_factory_t(); }

  static std::size_t constexpr default_bufsize()
  { return nb_inbuf_t::default_bufsize; }

  static throughput_checker_settings_t constexpr
  default_throughput_checker_settings()
  { return throughput_checker_settings_t(); }

  dispatcher_config_t()
  : selector_factory_(default_selector_factory())
  , bufsize_(default_bufsize())
  , throughput_checker_settings_(default_throughput_checker_settings())
  { }

  selector_factory_t selector_factory_;
  std::size_t bufsize_;
  throughput_checker_settings_t throughput_checker_settings_;
};

struct CUTI_ABI dispatcher_t
{
  dispatcher_t(logging_context_t const& logging_context,
               dispatcher_config_t config);

  dispatcher_t(dispatcher_t const&) = delete;
  dispatcher_t& operator=(dispatcher_t const&) = delete;

  endpoint_t add_listener(endpoint_t const& endpoint, method_map_t const& map);
  
  void run();

  /*
   * Causes the current or next call to run() to return as soon as
   * possible.  This function is signal- and thread-safe.
   */
  void stop(int sig);

  ~dispatcher_t();

private :
  struct impl_t;
  std::unique_ptr<impl_t> impl_;
};

} // cuti

#endif
