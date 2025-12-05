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
  auto max_count = options_.bytes_per_line_;
  if(max_count < 1)
  {
    max_count = 1;
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
