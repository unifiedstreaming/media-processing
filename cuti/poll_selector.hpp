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

#ifndef CUTI_POLL_SELECTOR_HPP_
#define CUTI_POLL_SELECTOR_HPP_

#include "linkage.h"
#include "selector.hpp"

#include <memory>

namespace cuti
{

#ifndef _WIN32

CUTI_ABI
std::unique_ptr<selector_t> create_poll_selector();

#endif // !_WIN32

} // cuti

#endif
