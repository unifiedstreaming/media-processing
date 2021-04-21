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

#include "poll_selector.hpp"

#include <cassert>
#include <ostream>
#include <type_traits>

namespace cuti
{

selector_factory_t::selector_factory_t(
  std::string name,
  std::unique_ptr<selector_t>(*creator)())
: name_(std::move(name))
, creator_((assert(creator != nullptr), creator))
{ }

std::string const& selector_factory_t::name() const
{
  return name_;
}

std::unique_ptr<selector_t> selector_factory_t::operator()() const
{
  auto result = (*creator_)();
  assert(result != nullptr);
  return result;
}

selector_factory_t::~selector_factory_t()
{
  // dtor suppresses move ctor & move assignment op -> no empty state
  static_assert(!std::is_nothrow_move_constructible_v<selector_factory_t>);
  static_assert(!std::is_nothrow_move_assignable_v<selector_factory_t>);
}

std::ostream& operator<<(std::ostream& os, selector_factory_t const& factory)
{
  os << factory.name();
  return os;
}

std::vector<selector_factory_t> available_selector_factories()
{
  std::vector<selector_factory_t> result;

#ifndef _WIN32
  result.emplace_back("poll_selector", create_poll_selector);
#endif

  return result;
}

} // cuti
