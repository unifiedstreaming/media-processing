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

#include "quoted.hpp"

#include "charclass.hpp"

#include <streambuf>

namespace cuti
{

namespace // anonymous
{

void print_char_rep(std::streambuf& sb, int c)
{
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

} // anonymous

void quoted_char_t::print(std::streambuf& sb) const
{
  if(c_ == eof)
  {
    sb.sputc('e');
    sb.sputc('o');
    sb.sputc('f');
  }
  else
  {
    sb.sputc('\'');
    print_char_rep(sb, c_);
    sb.sputc('\'');
  }
}

void quoted_string_t::print(std::streambuf& sb) const &&
{
  sb.sputc('\"');

  for(const char* p = first_; p != last_; ++p)
  {
    print_char_rep(sb, std::char_traits<char>::to_int_type(*p));
  }

  sb.sputc('\"');
}

} // cuti
