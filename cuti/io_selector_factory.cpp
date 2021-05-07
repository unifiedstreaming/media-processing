/*
 * Copyright (C) 2021 CodeShop B.V.
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

#include "io_selector_factory.hpp"

#include "epoll_selector.hpp"
#include "kqueue_selector.hpp"
#include "poll_selector.hpp"
#include "select_selector.hpp"

#include <ostream>

namespace cuti
{

std::ostream& operator<<(std::ostream& os,
                         io_selector_factory_t const& factory)
{
  os << factory.name();
  return os;
}

std::vector<io_selector_factory_t> available_io_selector_factories()
{
  std::vector<io_selector_factory_t> result;

#if CUTI_HAS_POLL_SELECTOR
  result.emplace_back("poll", create_poll_selector);
#endif

#if CUTI_HAS_SELECT_SELECTOR
  result.emplace_back("select", create_select_selector);
#endif

#if CUTI_HAS_EPOLL_SELECTOR
  result.emplace_back("epoll", create_epoll_selector);
#endif

#if CUTI_HAS_KQUEUE_SELECTOR
  result.emplace_back("kqueue", create_kqueue_selector);
#endif

  return result;
}

} // cuti
