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

#include "config_file_reader.hpp"

#include "charclass.hpp"
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

config_file_reader_t::config_file_reader_t(std::string origin_prefix,
                                 std::streambuf& sb)
: origin_prefix_(std::move(origin_prefix))
, sb_(sb)
, line_(1)
, at_end_(false)
, argument_()
{
  this->advance();
}

bool config_file_reader_t::at_end() const
{
  return at_end_;
}

char const* config_file_reader_t::current_argument() const
{
  assert(!this->at_end());
  return argument_.c_str();
}

std::string config_file_reader_t::current_origin() const
{
  return origin_prefix_ + '(' + std::to_string(line_) + ')';
}

void config_file_reader_t::advance()
{
  assert(!this->at_end());

  /*
   * See 'Lexical structure' in the header file.
   */
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

  // clear current argument
  argument_.clear();

  // check for eof
  if(c == eof)
  {
    at_end_ = true;
    return;
  }

  // collect next argument
  while(!is_space(c) && c != '#' && c != eof)
  {
    switch(c)
    {
    case '\"' :
    case '\'' :
      // quoted string subargument
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
              builder << this->current_origin() << 
                ": unexpected end of line in quoted string";
              builder.explode();
            }
            break;
          case '\t' :
          case '\r' :
            {
              system_exception_builder_t builder;
              builder << this->current_origin() << 
                ": illegal character in quoted string";
              builder.explode();
            }
            break;
          default :
            break;
          }

          argument_.push_back(static_cast<char>(c2));
          c2 = sb_.snextc();  
        }
      }
      break;

    case '\\' :
      // backslash escape subargument
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
          builder << this->current_origin() << 
            ": unexpected end of line in backslash escape";
          builder.explode();
        }
        break;
      default :
        {
          system_exception_builder_t builder;
          builder << this->current_origin() << 
            ": unknown backslash escape";
          builder.explode();
        }
        break;
      }

      argument_.push_back(static_cast<char>(c));
      break;

    default :
      // character literal subargument
      argument_.push_back(static_cast<char>(c));
      break;
    }

    c = sb_.snextc();
  }
}

} // cuti
