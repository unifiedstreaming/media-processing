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

#include "linkage.h"

#include <memory>

namespace cuti
{

struct endpoint_t;
struct event_pipe_reader_t;
struct logging_context_t;
struct method_map_t;
struct selector_factory_t;

struct CUTI_ABI dispatcher_t
{
  dispatcher_t(logging_context_t const& logging_context,
               event_pipe_reader_t& control,
               selector_factory_t const& selector_factory);

  dispatcher_t(dispatcher_t const&) = delete;
  dispatcher_t& operator=(dispatcher_t const&) = delete;

  void add_listener(endpoint_t const& endpoint, method_map_t const& map);
  
  void run();

  ~dispatcher_t();

private :
  struct impl_t;
  std::unique_ptr<impl_t> impl_;
};

} // cuti

#endif
