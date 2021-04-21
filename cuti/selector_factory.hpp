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

#ifndef CUTI_SELECTOR_FACTORY_HPP_
#define CUTI_SELECTOR_FACTORY_HPP_

#include "linkage.h"
#include "selector.hpp"
#include "socket_nifty.hpp"

#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

namespace cuti
{

struct CUTI_ABI selector_factory_t
{
  selector_factory_t(std::string name,
                     std::unique_ptr<selector_t>(*creator)());

  std::string const& name() const;
  std::unique_ptr<selector_t> operator()() const;

  ~selector_factory_t();

private :
  std::string name_;
  std::unique_ptr<selector_t>(*creator_)();
};

CUTI_ABI
std::ostream& operator<<(std::ostream& os, selector_factory_t const& factory);

CUTI_ABI
std::vector<selector_factory_t> available_selector_factories();

} // cuti

#endif
