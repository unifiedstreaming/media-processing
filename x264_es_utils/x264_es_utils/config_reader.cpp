/*
 * Copyright (C) 2021-2024 CodeShop B.V.
 *
 * This file is part of the x264_es_utils library.
 *
 * The x264_es_utils library is free software: you can redistribute it
 * and/or modify it under the terms of version 2 of the GNU General
 * Public License as published by the Free Software Foundation.
 *
 * The x264_es_utils library is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See version 2 of the GNU General Public License for more details.
 *
 * You should have received a copy of version 2 of the GNU General
 * Public License along with the x264_es_utils library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "config_reader.hpp"

#include "config.hpp"

namespace x264_es_utils
{

std::unique_ptr<cuti::service_config_t>
config_reader_t::read_config(int argc, char const* const argv[]) const
{
  return std::make_unique<config_t>(argc, argv);
}

} // x264_es_utils
