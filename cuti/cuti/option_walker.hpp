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
#include <type_traits>
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
 * result of <reader>.current_origin() to indicate the source of the
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
   * Tries to match the current state of the option walker to the
   * supplied option name and target.
   *
   * Precondition: !done().
   *
   * The target type is either a callable or an assignable lvalue.
   *
   * If the target is a callable that takes two arguments (an option
   * name and a constant args_reader_t reference), or if the target is
   * an lvalue of type flag_t, the option is treated as a flag
   * (boolean) option that can't carry any additional value.
   *
   * If the target is a callable that takes three arguments (an option
   * name, a constant args_reader_t reference, and a value string), or
   * if the target is an lvalue of some non-flag_t type, the option
   * requires a value.
   *
   * On a match, either the target is invoked (for a callable target),
   * or the lvalue is assigned to. If the lvalue is a std::optional<T>,
   * a match sets the optional's inner value; if the lvalue is a
   * std::vector<T>, a match appends to the vector.
   *
   * To convert a user-supplied value string to an lvalue of type T,
   * the option_walker calls into an ADL-found overload of
   * parse_optval (see above). 
   *
   * Callable targets should implement any required value string
   * conversion by themselves. Like parse_optval(), callable targets
   * can report errors by throwing something derived from
   * std::exception with a descriptive error message that should
   * include the result of args_reader_t::current_origin() to indicate
   * the source of the error.
   *
   * Returns true when a match is found, in which case the
   * option_walker is either left in the done state, or ready to match
   * further options. If no match is found, false is returned and a
   * match with some other option name can be tried.
   */
  template<typename Target>
  bool match(char const* name, Target&& target)
  {
    assert(!this->done());

    bool result = false;
      
    if constexpr(std::is_invocable_v<Target&&,
      char const*, args_reader_t const&>)
    {
      // target does not take a value; option is a simple flag
      result = this->match_flag_target(name, std::forward<Target>(target));
    }
    else if constexpr(std::is_invocable_v<Target&&,
      char const*, args_reader_t const&, char const*>)
    {
      // target takes a value; option argument required
      result = this->match_value_target(name, std::forward<Target>(target));
    }
    else
    {
      // assume the target is an lvalue we can assign to
      static_assert(std::is_lvalue_reference_v<Target&&>,
        "option_walker: target type not supported");
      result = match_lvalue(name, std::forward<Target>(target));
    }

    return result;
  }

private :
  template<typename Target>
  bool match_flag_target(char const* name, Target&& target)
  {
    bool result = false;
    bool advance = false;
   
    if(option_walker_t::is_short_option(name))
    {
      if(this->short_option_ptr_ != nullptr &&
        *this->short_option_ptr_ == name[1])
      {
        result = true;
        std::forward<Target>(target)(name, reader_);

        ++this->short_option_ptr_;
        advance = *this->short_option_ptr_ == '\0';
      }
    }
    else if(option_walker_t::is_long_option(name))
    {
      char const* suffix =
        option_walker_t::match_prefix(reader_.current_argument(), name);
      if(suffix != nullptr && *suffix == '\0')
      {
        result = true;
        std::forward<Target>(target)(name, reader_);
        advance = true;
      }
    }

    if(advance)
    {
      this->reader_.advance();
      this->on_next_argument();
    }
    
    return result;
  }

  template<typename Target>
  bool match_value_target(char const* name, Target&& target)
  {
    char const* in = nullptr;
    bool result = this->value_option_matches(name, in);
    if(result)
    {
      assert(in != nullptr);
      std::forward<Target>(target)(name, reader_, in);

      this->reader_.advance();
      this->on_next_argument();
    }

    return result;
  }
  
  bool match_lvalue(char const* name, flag_t& value)
  {
    auto handler = [&value](char const*, args_reader_t const&)
    {
      value = true;
    };

    return this->match(name, handler);
  }

  template<typename T>
  bool match_lvalue(char const* name, std::optional<T>& value)
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
  bool match_lvalue(char const* name, std::vector<T>& value)
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
  bool match_lvalue(char const* name, T& value)
  {
    auto handler = [&value]
      (char const* name, args_reader_t const& reader, char const *in)
    {
      parse_optval(name, reader, in, value);
    };

    return this->match(name, handler);
  }

  bool value_option_matches(char const* name, char const*& value);
  void on_next_argument();

  static bool is_short_option(char const* name);
  static bool is_long_option(char const* name);
  static char const* match_prefix(char const* arg, char const* prefix);

private :
  args_reader_t& reader_;
  bool done_;
  char const* short_option_ptr_;
};

} // cuti

#endif
