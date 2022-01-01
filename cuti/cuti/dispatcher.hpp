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

#include "default_scheduler.hpp"
#include "linkage.h"
#include "listener.hpp"
#include "selector_factory.hpp"

#include <memory>
#include <string>
#include <vector>

namespace cuti
{

struct logging_context_t;
struct tcp_connection_t;

struct CUTI_ABI dispatcher_t
{
  dispatcher_t(logging_context_t const& logging_context,
               tcp_connection_t& control,
               selector_factory_t const& selector_factory);

  dispatcher_t(dispatcher_t const&) = delete;
  dispatcher_t& operator=(dispatcher_t const&) = delete;

  void add_listener(std::unique_ptr<listener_t> listener);
  
  void run();

private :
  void on_control();
  void on_listener(listener_t& listener);

private :
  logging_context_t const& logging_context_;
  tcp_connection_t& control_;
  std::string selector_name_;
  default_scheduler_t scheduler_;
  int sig_;
  std::vector<std::unique_ptr<listener_t>> listeners_;
};

} // cuti

#endif
