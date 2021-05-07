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

#ifndef CUTI_IO_SELECTOR_FACTORY_HPP_
#define CUTI_IO_SELECTOR_FACTORY_HPP_

#include "io_selector.hpp"
#include "linkage.h"
#include "socket_nifty.hpp"

#include <iosfwd>
#include <memory>
#include <vector>

namespace cuti
{

struct CUTI_ABI io_selector_factory_t
{
  template<int N>
  io_selector_factory_t(char const(&name)[N],
                        std::unique_ptr<io_selector_t>(&creator)())
  : name_(name)
  , creator_(creator)
  { }

  char const* name() const noexcept
  { return name_; }

  std::unique_ptr<io_selector_t> operator()() const
  { return (*creator_)(); }

private :
  char const* name_;
  std::unique_ptr<io_selector_t>(*creator_)();
};

CUTI_ABI
std::ostream& operator<<(std::ostream& os,
                         io_selector_factory_t const& factory);

CUTI_ABI
std::vector<io_selector_factory_t> available_io_selector_factories();

} // cuti

#endif
