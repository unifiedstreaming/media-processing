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
#include <string>

namespace xes
{

/*
 * Customization point for converting the string value for an option
 * called name to a value of type T. Must throw an exception with a
 * hopefully descriptive error message if the conversion fails; no
 * generic implementation provided.
 */
template<typename T>
T parse_optval(char const* name, char const* value);

/*
 * Predefined specializations; users may add additional
 * specializations for other types.
 */
template<>
int parse_optval<int>(char const* name, char const* value);

template<>
unsigned int parse_optval<unsigned int>(char const* name, char const* value);

template<>
std::string parse_optval<std::string>(char const* name, char const* value);

/*
 * Our option walker
 */
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
   * Tries to match name against the current command line option. If
   * name matches, the option value is stored in value, the walker
   * moves on to the potential next option, and true is returned. If
   * name does not match, value is left unchanged, the walker stays at
   * the current option, and false is returned.
   *      
   * A boolean value option is a simple flag that, if matched, simply
   * sets value to true. If another type of option is matched, value
   * is set to what is explictly specified on the command line by
   * calling one of the parse_optval() specializations.
   *
   * Precondition: !done().
   */
  bool match(char const* name, bool& value);

  template<typename T>
  bool match(char const* name, T& value)
  {
    char const* input = nullptr;
    bool result = this->do_match(name, input);
    if(result)
    {
      assert(input != nullptr);
      value = parse_optval<T>(name, input);
    }
    return result;
  }
    
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
  bool do_match(char const* name, char const*& value);
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
