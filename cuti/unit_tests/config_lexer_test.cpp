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

#include "config_lexer.hpp"

#include <exception>
#include <sstream>
#include <string>

// Enable assert()
#undef NDEBUG
#include <cassert>

namespace // anonymous
{

using namespace cuti;

void whitespace()
{
  std::string input = " \n\t\n\r\n";
  std::stringstream stream(input);
  config_lexer_t lexer("input", *stream.rdbuf());

  assert(lexer.at_eof());
  assert(lexer.current_line() == 4);
}

void comment()
{
  std::string input = "#Comment\n";
  std::stringstream stream(input);
  config_lexer_t lexer("comment", *stream.rdbuf());

  assert(lexer.at_eof());
  assert(lexer.current_line() == 2);
}

void whitespace_and_comments()
{
  std::string input = " \n\t#Comment\n\r\n#Comment\ntoken#Comment\n";
  std::stringstream stream(input);
  config_lexer_t lexer("input", *stream.rdbuf());

  assert(!lexer.at_eof());
  assert(lexer.current_line() == 5);
  assert(lexer.current_token() == "token");
  lexer.advance();

  lexer.at_eof();
  assert(lexer.current_line() == 6);
}

void multiple_tokens_on_separate_lines()
{
  std::string input = "one\ntwo\nthree\n";
  std::stringstream stream(input);
  config_lexer_t lexer("input", *stream.rdbuf());

  assert(!lexer.at_eof());
  assert(lexer.current_line() == 1);
  assert(lexer.current_token() == "one");
  lexer.advance();

  assert(!lexer.at_eof());
  assert(lexer.current_line() == 2);
  assert(lexer.current_token() == "two");
  lexer.advance();

  assert(!lexer.at_eof());
  assert(lexer.current_line() == 3);
  assert(lexer.current_token() == "three");
  lexer.advance();

  assert(lexer.at_eof());
  assert(lexer.current_line() == 4);
}

void multiple_tokens_on_single_line()
{
  std::string input = "one\ttwo\rthree four";
  std::stringstream stream(input);
  config_lexer_t lexer("input", *stream.rdbuf());

  assert(!lexer.at_eof());
  assert(lexer.current_line() == 1);
  assert(lexer.current_token() == "one");
  lexer.advance();

  assert(!lexer.at_eof());
  assert(lexer.current_line() == 1);
  assert(lexer.current_token() == "two");
  lexer.advance();

  assert(!lexer.at_eof());
  assert(lexer.current_line() == 1);
  assert(lexer.current_token() == "three");
  lexer.advance();

  assert(!lexer.at_eof());
  assert(lexer.current_line() == 1);
  assert(lexer.current_token() == "four");
  lexer.advance();

  assert(lexer.at_eof());
  assert(lexer.current_line() == 1);
}

void single_quoted_string_literal()
{
  std::string input = "\'C:\\Program Files\\Unified Streaming\'\n";
  std::stringstream stream(input);
  config_lexer_t lexer("input", *stream.rdbuf());

  assert(!lexer.at_eof());
  assert(lexer.current_line() == 1);
  assert(lexer.current_token() == "C:\\Program Files\\Unified Streaming");
  lexer.advance();

  assert(lexer.at_eof());
  assert(lexer.current_line() == 2);
}

void double_quote_in_single_quotes()
{
  std::string input = "\'\"Wowza Wowza Wowza!\"\'\n";
  std::stringstream stream(input);
  config_lexer_t lexer("input", *stream.rdbuf());

  assert(!lexer.at_eof());
  assert(lexer.current_line() == 1);
  assert(lexer.current_token() == "\"Wowza Wowza Wowza!\"");
  lexer.advance();

  assert(lexer.at_eof());
  assert(lexer.current_line() == 2);
}

void missing_single_quote()
{
  std::string input = "\'C:\\Program Files\\Unified Streaming\n";
  std::stringstream stream(input);

  bool caught = false;
  try
  {
    config_lexer_t lexer("input", *stream.rdbuf());
  }
  catch(std::exception const&)
  {
    caught = true;
  }
  assert(caught);
}

void double_quoted_string_literal()
{
  std::string input = "\"C:\\Program Files\\Unified Streaming\"\n";
  std::stringstream stream(input);
  config_lexer_t lexer("input", *stream.rdbuf());

  assert(!lexer.at_eof());
  assert(lexer.current_line() == 1);
  assert(lexer.current_token() == "C:\\Program Files\\Unified Streaming");
  lexer.advance();

  assert(lexer.at_eof());
  assert(lexer.current_line() == 2);
}

void single_quote_in_double_quotes()
{
  std::string input = "\"John O\'Mill\"\n";
  std::stringstream stream(input);
  config_lexer_t lexer("input", *stream.rdbuf());

  assert(!lexer.at_eof());
  assert(lexer.current_line() == 1);
  assert(lexer.current_token() == "John O\'Mill");
  lexer.advance();

  assert(lexer.at_eof());
  assert(lexer.current_line() == 2);
}

void missing_double_quote()
{
  std::string input = "\"C:\\Program Files\\Unified Streaming\n";
  std::stringstream stream(input);

  bool caught = false;
  try
  {
    config_lexer_t lexer("input", *stream.rdbuf());
  }
  catch(std::exception const&)
  {
    caught = true;
  }
  assert(caught);
}

void backslash_escapes()
{
  std::string input = "\\t\\n\\r\\ \\\"\\#\\\'\\\\\n";
  std::stringstream stream(input);
  config_lexer_t lexer("input", *stream.rdbuf());

  assert(!lexer.at_eof());
  assert(lexer.current_line() == 1);
  assert(lexer.current_token() == "\t\n\r \"#\'\\");
  lexer.advance();

  assert(lexer.at_eof());
  assert(lexer.current_line() == 2);
}

void unknown_escape()
{
  std::string input = "\\z";
  std::stringstream stream(input);

  bool caught = false;
  try
  {
    config_lexer_t lexer("input", *stream.rdbuf());
  }
  catch(std::exception const&)
  {
    caught = true;
  }
  assert(caught);
}

void token_concatenation()
{
  std::string input = "\"In and out of\"\\ quotes\n";
  std::stringstream stream(input);
  config_lexer_t lexer("input", *stream.rdbuf());

  assert(!lexer.at_eof());
  assert(lexer.current_line() == 1);
  assert(lexer.current_token() == "In and out of quotes");
  lexer.advance();

  assert(lexer.at_eof());
  assert(lexer.current_line() == 2);
}

} // anonymous

int main()
{
  whitespace();
  comment();
  whitespace_and_comments();

  multiple_tokens_on_separate_lines();
  multiple_tokens_on_single_line();

  single_quoted_string_literal();
  double_quote_in_single_quotes();
  missing_single_quote();

  double_quoted_string_literal();
  single_quote_in_double_quotes();
  missing_double_quote();

  backslash_escapes();
  unknown_escape();

  token_concatenation();
  
  return 0;
}
