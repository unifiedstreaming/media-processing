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

#ifndef CUTI_NB_STRING_SINK_HPP_
#define CUTI_NB_STRING_SINK_HPP_

#include "linkage.h"
#include "nb_sink.hpp"

#include <string>
#include <memory>

namespace cuti
{

CUTI_ABI std::unique_ptr<nb_sink_t>
make_nb_string_sink(std::shared_ptr<std::string> target);

} // cuti

#endif
