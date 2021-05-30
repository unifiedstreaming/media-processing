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

#ifndef CUTI_CONFIG_LEXER_HPP_
#define CUTI_CONFIG_LEXER_HPP_

#include "linkage.h"

#include <cassert>
#include <streambuf>
#include <string>

namespace cuti
{

/*
 * Lexical analyzer for convenience configuration files.  This class
 * yields string tokens intended for building a command line vector
 * with (additional) arguments read from a configuration file.
 *
 * Lexical structure
 * =================
 *
 * A configuration file is a sequence of zero or more tokens,
 * separated by whitespace and comments.
 *
 * Whitespace is a sequence of one or more space, tab, newline or
 * carriage return characters.
 *
 * A comment is a hash character followed by all the characters on the
 * line it is on.
 *
 * A token is the concatenation of one or more subtokens.
 *
 * A subtoken is either a quoted string, a backslash escape sequence,
 * or a character literal.
 *
 * Quoted strings
 * --------------
 *
 * A quoted string is a sequence of zero or more characters between
 * two matching single or double quotes. Between these quotes, any
 * character is allowed, except for the quote character itself, tab,
 * carriage return and newline. For Windows user convenience, a
 * backslash in a quoted string is treated as an ordinary character.
 *
 * Backslash escape sequences
 * --------------------------
 * 
 * Outside of the quoted string context, the following backslash
 * escape sequences are defined:
 *
 * \t       -> tab
 * \n       -> newline
 * \r       -> carriage return
 * \<space> -> space
 * \"       -> double quote
 * \#       -> hash
 * \'       -> single quote
 * \\       -> backslash
 *
 * A backslash followed by anything else is illegal.
 *
 * Character literals
 * ------------------
 *
 * Any other character is treated as a character literal representing
 * itself.
 */
struct CUTI_ABI config_lexer_t
{
  config_lexer_t(std::string origin, std::streambuf& sb);

  config_lexer_t(config_lexer_t const&) = delete;
  config_lexer_t& operator=(config_lexer_t const&) = delete;

  /*
   * Returns the origin as passed to the constructor.  This string
   * value, along with the current line number, is used to construct
   * error messages.
   */
  std::string const& origin() const noexcept
  {
    return origin_;
  }

  /*
   * Returns true when all tokens have been reported.
   */
  bool at_eof() const noexcept
  {
    return at_eof_;
  }

  /*
   * Returns the current line number.  
   */
  int current_line() const noexcept
  {
    return line_;
  }

  /*
   * Returns the current token; in principle, the empty string is a
   * valid token value.
   * PRE: !at_eof()
   */
  std::string const& current_token() const noexcept
  {
    assert(!this->at_eof());
    return token_;
  }

  /*
   * Advances to the next token.
   * PRE: !at_eof()
   */
  void advance();

private :
  std::string const origin_;
  std::streambuf& sb_;
  int line_;
  bool at_eof_;
  std::string token_;
};

} // cuti

#endif
