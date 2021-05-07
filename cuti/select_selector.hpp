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

#ifndef CUTI_SELECT_SELECTOR_HPP_
#define CUTI_SELECT_SELECTOR_HPP_

#include "io_selector.hpp"
#include "linkage.h"

#include <memory>

#define CUTI_HAS_SELECT_SELECTOR 1

#if CUTI_HAS_SELECT_SELECTOR

namespace cuti
{

CUTI_ABI
std::unique_ptr<io_selector_t> create_select_selector();

}

#endif // CUTI_HAS_SELECT_SELECTOR

#endif // CUTI_SELECT_SELECTOR_HPP_
