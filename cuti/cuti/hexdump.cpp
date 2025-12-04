/*
 * Copyright (C) 2025 CodeShop B.V.
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

#include "hexdump.hpp"

#include "charclass.hpp"

#include <cstddef>
#include <iomanip>

namespace cuti
{

void hexdump_t::print(std::ostream& os) const &&
{
  // calculate the max number of bytes per line 
  unsigned int available_width = 79;

  // subtract leading spaces
  if(available_width >= options_.leading_spaces_)
  {
    available_width -= options_.leading_spaces_;
  }
  else
  {
    available_width = 0;
  }

  // subtract width of offset field (8), the space after the offset
  // field (1), and the spaces between the hex values and the char
  // dump (2)
  if(available_width >= 11)
  {
    available_width -= 11;
  }
  else
  {
    available_width = 0;
  }

  // each byte we report requires 4 positions: 1 for the space before
  // the hex value, 2 for the hex value, and 1 for the char dump
  auto max_count = available_width / 4;

  // fix up if excessive
  if(max_count < 4)
  {
    max_count = 4;
  }
  else if(max_count > 16)
  {
    max_count = 16;
  }
  
  auto saved_flags = os.flags();
  os << std::hex << std::setfill('0') << std::uppercase << std::noshowbase;

  unsigned char const* first = this->first_;
  unsigned char const* last = this->last_;
  std::size_t offset = 0;

  while(first != last)
  {
    os << '\n';

    for(auto spaces = options_.leading_spaces_; spaces != 0; --spaces)
    {
      os << ' ';
    }

    os << std::setw(8) << offset;
    os << ' ';

    unsigned char const* p;
    std::size_t count;

    for(p = first, count = 0; p != last && count != max_count; ++p, ++count)
    {
      os << ' ' << std::setw(2) << int(*p);
    }

    for(; count != max_count; ++count)
    {
      os << "   ";
    }

    os << "  ";

    for(p = first, count = 0; p != last && count != max_count; ++p, ++count)
    {
      if(is_printable(*p))
      {
        os << *p;
      }
      else
      {
        os << '.';
      }
    }

    for(; count != max_count; ++count)
    {
      os << ' ';
    }

    first = p;
    offset += count;
  }

  os.flags(saved_flags);
}

} // cuti
