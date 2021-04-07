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

#ifndef CUTI_FS_UTILS_HPP_
#define CUTI_FS_UTILS_HPP_

#include "linkage.h"

#include <string>

namespace cuti
{

CUTI_ABI void rename_if_exists(char const* old_name, char const* new_name);
CUTI_ABI void delete_if_exists(char const* name);
CUTI_ABI std::string current_directory();
CUTI_ABI std::string absolute_path(char const* path);

} // cuti

#endif
