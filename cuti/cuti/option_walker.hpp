/*
 * Copyright (C) 2021-2025 CodeShop B.V.
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

#ifndef CUTI_OPTION_WALKER_HPP_
#define CUTI_OPTION_WALKER_HPP_

#include "args_reader.hpp"
#include "flag.hpp"
#include "linkage.h"

#include <cassert>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace cuti
{

/*
 * Convert the string value <in> for an option called <name> to a
 * value of the type of <out>. parse_optval() throws something derived
 * from std::exception with a descriptive error message if the
 * conversion fails.
 *
 * parse_optval() is a customization point: users may provide further
 * overloads, found by ADL, for other types of <out>. If the
 * conversion fails, the exception message string should include the
 * result of <reader>.current_origin() to pinpoint the origin of the
 * error.
 */
CUTI_ABI void parse_optval(char const* name, args_reader_t const& reader,
                           char const* in, short& out);
CUTI_ABI void parse_optval(char const* name, args_reader_t const& reader,
                           char const* in, unsigned short& out);
CUTI_ABI void parse_optval(char const* name, args_reader_t const& reader,
                           char const* in, int& out);
CUTI_ABI void parse_optval(char const* name, args_reader_t const& reader,
                           char const* in, unsigned int& out);
CUTI_ABI void parse_optval(char const* name, args_reader_t const& reader,
                           char const* in, long& out);
CUTI_ABI void parse_optval(char const* name, args_reader_t const& reader,
                           char const* in, unsigned long& out);
CUTI_ABI void parse_optval(char const* name, args_reader_t const& reader,
                           char const* in, long long& out);
CUTI_ABI void parse_optval(char const* name, args_reader_t const& reader,
                           char const* in, unsigned long long& out);

CUTI_ABI void parse_optval(char const* name, args_reader_t const& reader,
                           char const* in, std::string& out);

/*
 * Our option walker
 */
struct CUTI_ABI option_walker_t
{
  explicit option_walker_t(args_reader_t& reader);

  /*
   * Tells if all options have been matched.
   */
  bool done() const
  {
    return done_;
  }

  /*
   * Tries to match the option <name> against the current reader
   * argument. On a match, the option value is stored in <value>, the
   * walker moves on to the potentially next option, and true is
   * returned. If <name> does not match, <value> is left unchanged,
   * the walker stays at the current option, and false is returned.
   *
   * flag options take no explicit value from the command line: if a
   * flag option is matched, <value> is simply set to true. If
   * another type of option is matched, <value> is set to what is
   * specified on the command line by calling one of the
   * parse_optval() overloads.
   *
   * If <value> is a std::optional<T>, then on a match, the optional's
   * 'inner' value is set.  This helps if no obvious default value is
   * available for T.
   *
   * If <value> is a std::vector<T>, then on a match, a single value
   * of type T is appended to the vector.  This gives meaningful results
   * for repeated command line options.
   *
   * Precondition: !done().
   */
  bool match(char const* name, flag_t& value);

  template<typename T>
  bool match(char const* name, std::optional<T>& value)
  {
    T inner_value;
    if(!this->match(name, inner_value))
    {
      return false;
    }

    value = inner_value;
    return true;
  }

  template<typename T>
  bool match(char const* name, std::vector<T>& value)
  {
    T element;
    if(!this->match(name, element))
    {
      return false;
    }

    value.push_back(std::move(element));
    return true;
  }

  template<typename T>
  bool match(char const* name, T& value)
  {
    char const* in = nullptr;
    bool result = this->value_option_matches(name, in);
    if(result)
    {
      assert(in != nullptr);
      parse_optval(name, reader_, in, value);
      reader_.advance();
      on_next_argument();
    }
    return result;
  }

private :
  bool value_option_matches(char const* name, char const*& value);
  void on_next_argument();

private :
  args_reader_t& reader_;
  bool done_;
  char const* short_option_ptr_;
};

} // cuti

#endif
