/*
 * Copyright (C) 2021-2022 CodeShop B.V.
 *
 * This file is part of the x264_encoding_service.
 *
 * The x264_encoding_service is free software: you can redistribute it
 * and/or modify it under the terms of version 2 of the GNU General
 * Public License as published by the Free Software Foundation.
 *
 * The x264_encoding_service is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See version 2 of the GNU General Public License for more details.
 *
 * You should have received a copy of version 2 of the GNU General
 * Public License along with the x264_encoding_service.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef XES_UTILS_CONFIG_READER_HPP_
#define XES_UTILS_CONFIG_READER_HPP_

#include <cuti/service.hpp>

namespace xes_utils
{

struct config_reader_t : cuti::service_config_reader_t
{
  std::unique_ptr<cuti::service_config_t>
  read_config(int argc, char const* const argv[]) const override;
};

} // xes_utils
    
#endif
