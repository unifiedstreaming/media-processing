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

#include "selector_factory.hpp"

#include "kqueue_selector.hpp"
#include "poll_selector.hpp"
#include "select_selector.hpp"

#include <ostream>

namespace cuti
{

std::ostream& operator<<(std::ostream& os, selector_factory_t const& factory)
{
  os << factory.name();
  return os;
}

std::vector<selector_factory_t> available_selector_factories()
{
  std::vector<selector_factory_t> result;

#if defined(__APPLE__) || defined(__FreeBSD__)
  result.emplace_back("kqueue", create_kqueue_selector);
#endif
#ifndef _WIN32
  result.emplace_back("poll", create_poll_selector);
#endif
  result.emplace_back("select", create_select_selector);

  return result;
}

} // cuti
