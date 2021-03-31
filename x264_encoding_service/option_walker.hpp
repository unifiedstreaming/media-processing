/*
 * Copyright (C) 2021 CodeShop B.V.
 *
 * This file is part of the x264_encoding_service.
 *
 * The x264_encoding_service is free software: you can redistribute it
 * and/or modify it under the terms of version 2 of the GNU General
 * Public License as published by the Free Software Foundation.
 *
 * The x264_encoding_service is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See version 2 of the GNU General Public License for more details.
 *
 * You should have received a copy of version 2 of the GNU General
 * Public License along with the x264_encoding_service.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef XES_OPTION_WALKER_HPP_
#define XES_OPTION_WALKER_HPP_

#include <cassert>

namespace xes
{

struct option_walker_t
{
  option_walker_t(int argc, char const* const argv[]);

  /*
   * Tells if all options have been matched.
   */
  bool done() const
  {
    return done_;
  }

  /*
   * Tries to match the flag option named name.  Skips the current
   * option and returns true on success; stays at the current option
   * and returns false on failure.
   *
   * Precondition: !done().
   */
  bool match_flag(char const* name);

  /*
   * Tries to match the value option named name.  Skips the current
   * option and returns its value on success; stays at the current
   * option and returns nullptr on failure.
   *
   * Precondition: !done().
   */
  char const* match_value(char const* name);

  /*
   * Returns the index of the first non-option element in the argv
   * array passed to the constructor, or argc if there are no
   * non-option elements.
   * 
   * Precondition: done().
   */
  int next_index() const
  {
    assert(done_);
    return idx_;
  }

private :
  void on_next_element();

private :
  int argc_;
  char const* const* argv_;
  int idx_;
  bool done_;
  char const* short_option_ptr_;
};

} // xes

#endif
