/*
 * Copyright (C) 2021-2023 CodeShop B.V.
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

#include "remote_error.hpp"

#include <cassert>

namespace cuti
{

std::string remote_error_t::make_message(identifier_t const& type,
                                         std::string const& description)
{
  return "remote error: " + type.as_string() + ": " + description;
}

remote_error_t::rep_t::rep_t(identifier_t type, std::string description)
: type_((assert(type.is_valid()), std::move(type)))
, description_(std::move(description))
{ }

} // cuti
