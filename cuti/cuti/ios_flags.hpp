/*
 * Copyright (C) 2026 CodeShop B.V.
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

#ifndef CUTI_IOS_FLAGS_HPP_
#define CUTI_IOS_FLAGS_HPP_

#include "linkage.h"

#include <ios>

namespace cuti
{

/*
 * RAII helper for saving and restoring ios flags.
 */
struct CUTI_ABI ios_flags_t
{
  explicit ios_flags_t(std::ios_base& ios)
  : ios_(ios)
  , flags_(ios_.flags())
  { }

  ios_flags_t(ios_flags_t const&) = delete;
  ios_flags_t& operator=(ios_flags_t const&) = delete;

  ~ios_flags_t()
  { ios_.flags(flags_); }

private:
  std::ios_base& ios_;
  std::ios_base::fmtflags flags_;
};

} // cuti

#endif
