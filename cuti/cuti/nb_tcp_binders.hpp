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

#ifndef CUTI_NB_TCP_BINDERS_HPP_
#define CUTI_NB_TCP_BINDERS_HPP_

#include "linkage.h"
#include "nb_sink.hpp"
#include "nb_source.hpp"
#include "tcp_connection.hpp"

#include <memory>
#include <utility>

namespace cuti
{

CUTI_ABI std::pair<std::unique_ptr<nb_source_t>, std::unique_ptr<nb_sink_t>>
make_nb_tcp_binders(std::unique_ptr<tcp_connection_t> conn);

} // cuti

#endif
