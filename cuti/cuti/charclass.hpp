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

#ifndef CUTI_CHARCLASS_HPP_
#define CUTI_CHARCLASS_HPP_

#include "linkage.h"

/*
 * Some character (classification) utilities
 */
namespace cuti
{

CUTI_ABI
inline bool is_whitespace(int c)
{
  return c == '\t' || c == '\r' || c == ' ';
}

CUTI_ABI
inline bool is_printable(int c)
{
  return c >= 0x20 && c <= 0x7F;
}

CUTI_ABI
inline int digit_value(int c)
{
  return c >= '0' && c <= '9' ? c - '0' :
         -1;
}

CUTI_ABI
inline int hex_digit_value(int c)
{
  return c >= '0' && c <= '9' ? c - '0' :
         c >= 'A' && c <= 'F' ? c - 'A' + 10 :
         c >= 'a' && c <= 'f' ? c - 'a' + 10 :
         -1;
}

char constexpr hex_digits[] = {
  '0', '1', '2', '3', '4', '5', '6', '7',
  '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
};

} // cuti

#endif
