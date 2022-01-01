/*
 * Copyright (C) 2021-2022 CodeShop B.V.
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

#ifndef CUTI_CMDLINE_READER_HPP_
#define CUTI_CMDLINE_READER_HPP_

#include "args_reader.hpp"
#include "linkage.h"

namespace cuti
{

/*
 * Command line program argument reader.
 */
struct CUTI_ABI cmdline_reader_t : args_reader_t
{
  cmdline_reader_t(int argc, char const* const argv[]);

  bool at_end() const override;
  char const* current_argument() const override;
  std::string current_origin() const override;
  void advance() override;  

private :
  int argc_;
  char const* const* argv_;
  int idx_;
};

} // cuti

#endif
