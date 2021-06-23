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

#include "args_reader.hpp"
#include "epoll_selector.hpp"
#include "kqueue_selector.hpp"
#include "poll_selector.hpp"
#include "select_selector.hpp"
#include "system_error.hpp"

#include <algorithm>
#include <cstring>
#include <ostream>

namespace cuti
{

selector_factory_t::selector_factory_t()
: selector_factory_t(available_selector_factories().front())
{ }

std::ostream& operator<<(std::ostream& os, selector_factory_t const& factory)
{
  os << factory.name();
  return os;
}

std::vector<selector_factory_t> available_selector_factories()
{
  std::vector<selector_factory_t> result;

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

CUTI_ABI
void parse_optval(char const* name, args_reader_t const& reader,
                  char const* in, selector_factory_t& out)
{
  auto factories = available_selector_factories();
  auto name_matches = [&](selector_factory_t const& factory)
   { return std::strcmp(in, factory.name()) == 0; };

  auto pos = std::find_if(factories.begin(), factories.end(), name_matches);
  if(pos == factories.end())
  {
    system_exception_builder_t builder;
    builder << reader.current_origin() << ": " <<
      "invalid selector type '" << in << "'. Valid types are: ";
    auto pos = factories.begin();
    builder << *pos;
    for(++pos; pos != factories.end(); ++pos)
    {
      builder << ", " << *pos;
    }
    builder << ".";
    builder.explode();
  }

  out = *pos;
}

} // cuti
