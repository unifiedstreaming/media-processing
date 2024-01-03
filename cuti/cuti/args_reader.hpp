/*
 * Copyright (C) 2021-2024 CodeShop B.V.
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

#ifndef CUTI_ARGS_READER_HPP_
#define CUTI_ARGS_READER_HPP_

#include "linkage.h"

#include <string>

namespace cuti
{

/*
 * Abstract program argument reader interface.
 */
struct CUTI_ABI args_reader_t
{
  args_reader_t();

  args_reader_t(args_reader_t const&) = delete;
  args_reader_t& operator=(args_reader_t const&) = delete;

  /*
   * Returns true when no more arguments remain, false otherwise.
   */
  virtual bool at_end() const = 0;

  /*
   * Returns the current argument.
   * PRE: !this->at_end().
   */
  virtual char const* current_argument() const = 0;

  /*
   * Returns the origin of the current argument, or, if at_end() is
   * true, the origin of the end of the argument list. Intended for
   * generating diagnotics.
   */
  virtual std::string current_origin() const = 0;

  /*
   * Advances to the next argument.
   * PRE: !this->at_end().
   */
  virtual void advance() = 0;

  virtual ~args_reader_t();
};

} // cuti

#endif
