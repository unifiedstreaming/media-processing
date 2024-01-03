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

#ifndef CUTI_CONFIG_FILE_READER_HPP_
#define CUTI_CONFIG_FILE_READER_HPP_

#include "args_reader.hpp"
#include "linkage.h"

#include <cassert>
#include <streambuf>
#include <string>

namespace cuti
{

/*
 * Configuration file program argument reader.
 *
 * Lexical structure
 * =================
 *
 * A configuration file is a sequence of zero or more arguments,
 * separated by whitespace and comments.
 *
 * Whitespace is a sequence of one or more space, tab, newline or
 * carriage return characters.
 *
 * A comment is a hash character followed by all the characters on the
 * line it is on.
 *
 * An argument is the concatenation of one or more subarguments.
 *
 * A subargument is either a quoted string, a backslash escape
 * sequence, or a character literal.
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
struct CUTI_ABI config_file_reader_t : args_reader_t
{
  config_file_reader_t(std::string origin_prefix, std::streambuf& sb);

  bool at_end() const override;
  char const* current_argument() const override;
  std::string current_origin() const override;
  void advance() override;  

private :
  std::string const origin_prefix_;
  std::streambuf& sb_;
  int line_;
  bool at_end_;
  std::string argument_;
};

} // cuti

#endif
