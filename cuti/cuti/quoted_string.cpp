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

#include "quoted_string.hpp"

#include "charclass.hpp"

#include <streambuf>

namespace cuti
{

void quoted_string_t::print(std::streambuf& sb) const &&
{
  static char constexpr hex_digits[] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
  };

  sb.sputc('\"');

  for(const char* p = first_; p != last_; ++p)
  {
    int c = std::char_traits<char>::to_int_type(*p);

    switch(c)
    {
    case '\t' :
      sb.sputc('\\'); sb.sputc('t');
      break;
    case '\n' :
      sb.sputc('\\'); sb.sputc('n');
      break;
    case '\r' :
      sb.sputc('\\'); sb.sputc('r');
      break;
    case '\"' :
      sb.sputc('\\'); sb.sputc('\"');
      break;
    case '\'' :
      sb.sputc('\\'); sb.sputc('\'');
      break;
    case '\\' :
      sb.sputc('\\'); sb.sputc('\\');
      break;
    default :
      if(is_printable(c))
      {
        sb.sputc(c);
      }
      else
      {
        sb.sputc('\\'); sb.sputc('x');
        sb.sputc(hex_digits[(c >> 4) & 0x0F]);
        sb.sputc(hex_digits[c & 0x0F]);
      }
      break;
    }
  }

  sb.sputc('\"');
}

} // cuti
