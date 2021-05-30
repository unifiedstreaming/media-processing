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
#include "system_error.hpp"

#include <utility>

namespace cuti
{

namespace // anoynymous
{

bool is_space(int c)
{
  switch(c)
  {
  case '\t' :
  case '\n' :
  case '\r' :
  case ' ' :
    return true;
  default :
    return false;
  }
}

} // anonymous

config_lexer_t::config_lexer_t(std::string origin, std::streambuf& sb)
: origin_(std::move(origin))
, sb_(sb)
, line_(1)
, at_eof_(false)
, token_()
{
  this->advance();
}

void config_lexer_t::advance()
{
  /*
   * See 'Lexical structure' in the header file.
   */
  using traits_type = std::streambuf::traits_type;
  static auto constexpr eof = traits_type::eof();

  assert(!this->at_eof());

  int c = sb_.sgetc();

  // skip spaces and comments
  while(c == '#' || is_space(c))
  {
    switch(c)
    {
    case '#' :
      // comment: skip until end of line
      do
      {
        c = sb_.snextc();
      } while(c != '\n' && c != eof);
      break;

    case '\n' :
      // to next line
      ++line_;
      c = sb_.snextc();
      break;

    default :
      // discard
      c = sb_.snextc();
      break;
    }
  }

  // clear current token
  token_.clear();

  // check for eof
  if(c == eof)
  {
    at_eof_ = true;
    return;
  }

  // collect next token
  while(!is_space(c) && c != '#' && c != eof)
  {
    switch(c)
    {
    case '\"' :
    case '\'' :
      // quoted string subtoken
      {
        int c2 = sb_.snextc();
        while(c2 != c)
        {
          switch(c2)
          {
          case '\n' :
          case eof :
            {
              system_exception_builder_t builder;
              builder << origin_ << '(' << line_ <<
                "): unexpected end of line in quoted string";
              builder.explode();
            }
            break;
          case '\t' :
          case '\r' :
            {
              system_exception_builder_t builder;
              builder << origin_ << '(' << line_ <<
                "): illegal character in quoted string";
              builder.explode();
            }
            break;
          default :
            break;
          }

          token_.push_back(static_cast<char>(c2));
          c2 = sb_.snextc();  
        }
      }
      break;

    case '\\' :
      // backslash escape subtoken
      c = sb_.snextc();
      switch(c)
      {
      case 't' :
        c = '\t';
        break;
      case 'n' :
        c = '\n';
        break;
      case 'r' :
        c = '\r';
        break;
      case ' ' :
      case '\"' :
      case '#' :
      case '\'' :
      case '\\' :
        // no translation
        break ;
      case '\n' :
      case eof :
        {
          system_exception_builder_t builder;
          builder << origin_ << '(' << line_ <<
            "): unexpected end of line in backslash escape";
          builder.explode();
        }
        break;
      default :
        {
          system_exception_builder_t builder;
          builder << origin_ << '(' << line_ <<
            "): unknown backslash escape";
          builder.explode();
        }
        break;
      }

      token_.push_back(static_cast<char>(c));
      break;

    default :
      // character literal subtoken
      token_.push_back(static_cast<char>(c));
      break;
    }

    c = sb_.snextc();
  }
}

} // cuti
