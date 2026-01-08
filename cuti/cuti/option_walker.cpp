/*
 * Copyright (C) 2021-2026 CodeShop B.V.
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

#include "option_walker.hpp"

#include "system_error.hpp"

#include <cassert>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace cuti
{

namespace // anonymous
{

template<typename T>
T parse_unsigned(char const* name, args_reader_t const& reader,
                 char const* value, T max)
{
  static_assert(std::is_unsigned_v<T>);

  T result = 0;

  do
  {
    if(*value < '0' || *value > '9')
    {
      system_exception_builder_t builder;
      builder << reader.current_origin() <<
        ": digit expected in option value for '" << name << "'";
      builder.explode();
    }

    T digit = *value - '0';
    if(result > max / 10 || digit > max - 10 * result)
    {
      system_exception_builder_t builder;
      builder << reader.current_origin() <<
        ": overflow in option value for '" << name << "'";
      builder.explode();
    }

    result *= 10;
    result += digit;

    ++value;
  } while(*value != '\0');

  return result;
}

template<typename T>
void parse_signed_optval(char const* name, args_reader_t const& reader,
                         char const* in, T& out)
{
  static_assert(std::is_integral_v<T>);
  static_assert(std::is_signed_v<T>);
  using UT = std::make_unsigned_t<T>;

  static UT constexpr max = std::numeric_limits<T>::max();

  if(*in == '-')
  {
    UT unsigned_value =
      parse_unsigned<UT>(name, reader, in + 1, max + 1);
    T signed_value;

    if(unsigned_value != 0)
    {
      --unsigned_value;
      signed_value = unsigned_value;
      signed_value = -signed_value;
      --signed_value;
    }
    else
    {
      signed_value = unsigned_value;
    }

    out = signed_value;
  }
  else
  {
    out = parse_unsigned<UT>(name, reader, in, max);
  }
}  
  
template<typename T>
void parse_unsigned_optval(char const* name, args_reader_t const& reader,
                           char const* in, T& out)
{
  static_assert(std::is_unsigned_v<T>);

  static T constexpr max = std::numeric_limits<T>::max();
  out = parse_unsigned<T>(name, reader, in, max);
}

} // anonymous

void parse_optval(char const* name, args_reader_t const& reader,
                  char const* in, short& out)
{
  parse_signed_optval(name, reader, in, out);
}

void parse_optval(char const* name, args_reader_t const& reader,
                  char const* in, unsigned short& out)
{
  parse_unsigned_optval(name, reader, in, out);
}

void parse_optval(char const* name, args_reader_t const& reader,
                  char const* in, int& out)
{
  parse_signed_optval(name, reader, in, out);
}

void parse_optval(char const* name, args_reader_t const& reader,
                  char const* in, unsigned int& out)
{
  parse_unsigned_optval(name, reader, in, out);
}

void parse_optval(char const* name, args_reader_t const& reader,
                  char const* in, long& out)
{
  parse_signed_optval(name, reader, in, out);
}

void parse_optval(char const* name, args_reader_t const& reader,
                  char const* in, unsigned long& out)
{
  parse_unsigned_optval(name, reader, in, out);
}

void parse_optval(char const* name, args_reader_t const& reader,
                  char const* in, long long& out)
{
  parse_signed_optval(name, reader, in, out);
}

void parse_optval(char const* name, args_reader_t const& reader,
                  char const* in, unsigned long long& out)
{
  parse_unsigned_optval(name, reader, in, out);
}

void parse_optval(char const*, args_reader_t const&,
                  char const* in, std::string& out)
{
  out = in;
}

option_walker_t::option_walker_t(args_reader_t& reader)
: reader_(reader)
, done_(false)
, short_option_ptr_(nullptr)
{
  on_next_argument();
}

bool option_walker_t::value_option_matches(char const* name,
                                           char const*& value)
{
  assert(!done_);

  bool result = false;

  if(is_short_option(name) || is_long_option(name))
  {
    char const* suffix = match_prefix(reader_.current_argument(), name);
    if(suffix != nullptr)
    {
      if(*suffix == '=')
      {
        value = suffix + 1;
        result = true;
      }
      else if(*suffix == '\0')
      {
        reader_.advance();
        if(reader_.at_end())
        {
          system_exception_builder_t builder;
          builder << reader_.current_origin() << ": option \'" <<
            name << "\' requires a value";
          builder.explode();
        }

        value = reader_.current_argument();
        result = true;
      }
    }
  }

  return result;
}

void option_walker_t::on_next_argument()
{
  short_option_ptr_ = nullptr;

  if(reader_.at_end())
  {
    // out of arguments
    done_ = true;
  }
  else
  {
    char const* arg = reader_.current_argument();
    if(arg[0] != '-' || arg[1] == '\0')
    {
      // not an option
      done_ = true;
    }
    else if(arg[1] != '-')
    {
      // short option(s) found
      short_option_ptr_ = &arg[1];
    }
    else if(arg[2] == '\0')
    {
      // end of options marker
      done_ = true;
      reader_.advance();
    }
    else
    {
      // long option found
    }
  }
}

bool option_walker_t::is_short_option(char const *name)
{
  return name[0] == '-' &&
    name[1] != '\0' && name[1] != '-' &&
    name[2] == '\0';
}

bool option_walker_t::is_long_option(char const* name)
{
  return name[0] == '-' &&
    name[1] == '-' &&
    name[2] != '\0';
}

char const* option_walker_t::match_prefix(char const* arg, char const* prefix)
{
  while(*prefix == '-')
  {
    if(*arg != '-')
    {
      return nullptr;
    }
    ++arg;
    ++prefix;
  }

  while(*prefix != '\0')
  {
    if(*prefix != *arg &&
       !(*prefix == '-' && *arg == '_') &&
       !(*prefix == '_' && *arg == '-'))
    {
      return nullptr;
    }
    ++arg;
    ++prefix;
  }

  return arg;
}

} // cuti
