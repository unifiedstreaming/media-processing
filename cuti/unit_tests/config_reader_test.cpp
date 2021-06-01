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

#include "config_reader.hpp"
#include "viewbuf.hpp"

#include <cstring>
#include <exception>
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
  viewbuf_t buf(input.data(), input.data() + input.size());
  config_reader_t reader("input", buf);

  assert(reader.at_end());
  assert(reader.current_origin() == "input(4)");
}

void comment()
{
  std::string input = "#Comment\n";
  viewbuf_t buf(input.data(), input.data() + input.size());
  config_reader_t reader("input", buf);

  assert(reader.at_end());
  assert(reader.current_origin() == "input(2)");
}

void whitespace_and_comments()
{
  std::string input = " \n\t#Comment\n\r\n#Comment\ntoken#Comment\n";
  viewbuf_t buf(input.data(), input.data() + input.size());
  config_reader_t reader("input", buf);

  assert(!reader.at_end());
  assert(reader.current_origin() == "input(5)");
  assert(std::strcmp(reader.current_argument(), "token") == 0);
  reader.advance();

  reader.at_end();
  assert(reader.current_origin() == "input(6)");
}

void multiple_tokens_on_separate_lines()
{
  std::string input = "one\ntwo\nthree\n";
  viewbuf_t buf(input.data(), input.data() + input.size());
  config_reader_t reader("input", buf);

  assert(!reader.at_end());
  assert(reader.current_origin() == "input(1)");
  assert(std::strcmp(reader.current_argument(), "one") == 0);
  reader.advance();

  assert(!reader.at_end());
  assert(reader.current_origin() == "input(2)");
  assert(std::strcmp(reader.current_argument(), "two") == 0);
  reader.advance();

  assert(!reader.at_end());
  assert(reader.current_origin() == "input(3)");
  assert(std::strcmp(reader.current_argument(), "three") == 0);
  reader.advance();

  assert(reader.at_end());
  assert(reader.current_origin() == "input(4)");
}

void multiple_tokens_on_single_line()
{
  std::string input = "one\ttwo\rthree four";
  viewbuf_t buf(input.data(), input.data() + input.size());
  config_reader_t reader("input", buf);

  assert(!reader.at_end());
  assert(reader.current_origin() == "input(1)");
  assert(std::strcmp(reader.current_argument(), "one") == 0);
  reader.advance();

  assert(!reader.at_end());
  assert(reader.current_origin() == "input(1)");
  assert(std::strcmp(reader.current_argument(), "two") == 0);
  reader.advance();

  assert(!reader.at_end());
  assert(reader.current_origin() == "input(1)");
  assert(std::strcmp(reader.current_argument(), "three") == 0);
  reader.advance();

  assert(!reader.at_end());
  assert(reader.current_origin() == "input(1)");
  assert(std::strcmp(reader.current_argument(), "four") == 0);
  reader.advance();

  assert(reader.at_end());
  assert(reader.current_origin() == "input(1)");
}

void single_quoted_string_literal()
{
  std::string input = "\'C:\\Program Files\\Unified Streaming\'\n";
  viewbuf_t buf(input.data(), input.data() + input.size());
  config_reader_t reader("input", buf);

  assert(!reader.at_end());
  assert(reader.current_origin() == "input(1)");
  assert(std::strcmp(reader.current_argument(),
    "C:\\Program Files\\Unified Streaming") == 0);
  reader.advance();

  assert(reader.at_end());
  assert(reader.current_origin() == "input(2)");
}

void double_quote_in_single_quotes()
{
  std::string input = "\'\"Wowza Wowza Wowza!\"\'\n";
  viewbuf_t buf(input.data(), input.data() + input.size());
  config_reader_t reader("input", buf);

  assert(!reader.at_end());
  assert(reader.current_origin() == "input(1)");
  assert(std::strcmp(reader.current_argument(), "\"Wowza Wowza Wowza!\"") == 0);
  reader.advance();

  assert(reader.at_end());
  assert(reader.current_origin() == "input(2)");
}

void missing_single_quote()
{
  std::string input = "\'C:\\Program Files\\Unified Streaming\n";
  viewbuf_t buf(input.data(), input.data() + input.size());

  bool caught = false;
  try
  {
    config_reader_t reader("input", buf);
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
  viewbuf_t buf(input.data(), input.data() + input.size());
  config_reader_t reader("input", buf);

  assert(!reader.at_end());
  assert(reader.current_origin() == "input(1)");
  assert(std::strcmp(reader.current_argument(), 
    "C:\\Program Files\\Unified Streaming") == 0);
  reader.advance();

  assert(reader.at_end());
  assert(reader.current_origin() == "input(2)");
}

void single_quote_in_double_quotes()
{
  std::string input = "\"John O\'Mill\"\n";
  viewbuf_t buf(input.data(), input.data() + input.size());
  config_reader_t reader("input", buf);

  assert(!reader.at_end());
  assert(reader.current_origin() == "input(1)");
  assert(std::strcmp(reader.current_argument(), "John O\'Mill") == 0);
  reader.advance();

  assert(reader.at_end());
  assert(reader.current_origin() == "input(2)");
}

void missing_double_quote()
{
  std::string input = "\"C:\\Program Files\\Unified Streaming\n";
  viewbuf_t buf(input.data(), input.data() + input.size());

  bool caught = false;
  try
  {
    config_reader_t reader("input", buf);
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
  viewbuf_t buf(input.data(), input.data() + input.size());
  config_reader_t reader("input", buf);

  assert(!reader.at_end());
  assert(reader.current_origin() == "input(1)");
  assert(std::strcmp(reader.current_argument(), "\t\n\r \"#\'\\") == 0);
  reader.advance();

  assert(reader.at_end());
  assert(reader.current_origin() == "input(2)");
}

void unknown_escape()
{
  std::string input = "\\z";
  viewbuf_t buf(input.data(), input.data() + input.size());

  bool caught = false;
  try
  {
    config_reader_t reader("input", buf);
  }
  catch(std::exception const&)
  {
    caught = true;
  }
  assert(caught);
}

void subargument_concatenation()
{
  std::string input = "\"In and out of\"\\ quotes\n";
  viewbuf_t buf(input.data(), input.data() + input.size());
  config_reader_t reader("input", buf);

  assert(!reader.at_end());
  assert(reader.current_origin() == "input(1)");
  assert(std::strcmp(reader.current_argument(), "In and out of quotes") == 0);
  reader.advance();

  assert(reader.at_end());
  assert(reader.current_origin() == "input(2)");
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

  subargument_concatenation();
  
  return 0;
}
