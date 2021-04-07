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

#include "option_walker.hpp"

#include "exception_builder.hpp"

#include <cassert>
#include <limits>
#include <stdexcept>

namespace xes
{

namespace // anonymous
{

bool is_short_option(char const *name)
{
  return name[0] == '-' &&
    name[1] != '\0' && name[1] != '-' &&
    name[2] == '\0';
}

bool is_long_option(char const* name)
{
  return name[0] == '-' &&
    name[1] == '-' &&
    name[2] != '\0';
}

char const* match_prefix(char const* elem, char const* prefix)
{
  while(*prefix == '-')
  {
    if(*elem != '-')
    {
      return nullptr;
    }
    ++elem;
    ++prefix;
  }

  while(*prefix != '\0')
  {
    if(*prefix != *elem &&
       !(*prefix == '-' && *elem == '_') &&
       !(*prefix == '_' && *elem == '-'))
    {
      return nullptr;
    }
    ++elem;
    ++prefix;
  }

  return elem;
}

unsigned int parse_unsigned(char const* name, char const* value,
                            unsigned int max)
{
  unsigned int result = 0;

  do
  {
    if(*value < '0' || *value > '9')
    {
      exception_builder_t<std::runtime_error> builder;
      builder << "digit expected in option value for " << name;
      builder.explode();
    }

    unsigned int digit = *value - '0';
    if(result > max / 10 || digit > max - 10 * result)
    {
      exception_builder_t<std::runtime_error> builder;
      builder << "overflow in option value for " << name;
      builder.explode();
    }

    result *= 10;
    result += digit;

    ++value;
  } while(*value != '\0');

  return result;
}
    
} // anonymous

void parse_optval(char const* name, char const* in, int& out)
{
  static unsigned int const max = std::numeric_limits<int>::max();

  if(*in == '-')
  {
    unsigned int unsigned_value = parse_unsigned(name, in + 1, max + 1);
    --unsigned_value;

    int signed_value = unsigned_value;
    signed_value = -signed_value;
    --signed_value;

    out = signed_value;
  }
  else
  {
    out = parse_unsigned(name, in, max);
  }
}

void parse_optval(char const* name, char const* in, unsigned int& out)
{
  static unsigned int const max = std::numeric_limits<unsigned int>::max();
  out = parse_unsigned(name, in, max);
}

void parse_optval(char const*, char const* in, std::string& out)
{
  out = in;
}

option_walker_t::option_walker_t(int argc, char const* const argv[])
: argc_(argc)
, argv_(argv)
, idx_(argc_ < 1 ? argc_ : 1)
, done_(false)
, short_option_ptr_(nullptr)
{
  on_next_element();
}

bool option_walker_t::match(const char *name, bool& value)
{
  assert(!done_);

  bool result = false;

  if(is_short_option(name))
  {
    if(short_option_ptr_ != nullptr && *short_option_ptr_ == name[1])
    {
      value = true;
      result = true;

      ++short_option_ptr_;
      if(*short_option_ptr_ == '\0')
      {
        ++idx_;
        on_next_element();
      }
    }
  }
  else if(is_long_option(name))
  {
    char const* suffix = match_prefix(argv_[idx_], name);
    if(suffix != nullptr && *suffix == '\0')
    {
      value = true;
      result = true;

      ++idx_;
      on_next_element();
    }
  }
    
  return result;
}

bool option_walker_t::do_match(char const* name, char const*& value)
{
  assert(!done_);

  bool result = false;

  if(is_short_option(name) || is_long_option(name))
  {
    char const* suffix = match_prefix(argv_[idx_], name);
    if(suffix != nullptr)
    {
      if(*suffix == '=')
      {
        value = suffix + 1;
        result = true;

        ++idx_;
        on_next_element();
      }
      else if(*suffix == '\0' && idx_ + 1 != argc_)
      {
        ++idx_;
        value = argv_[idx_];
        result = true;

        ++idx_;
        on_next_element();
      }
    }
  }

  return result;
}
        
void option_walker_t::on_next_element()
{
  short_option_ptr_ = nullptr;

  if(idx_ == argc_)
  {
    // out of elements
    done_ = true;
  }
  else
  {
    char const* elem = argv_[idx_];
    if(elem[0] != '-' || elem[1] == '\0')
    {
      // not an option
      done_ = true;
    }
    else if(elem[1] != '-')
    {
      // short option(s) found
      short_option_ptr_ = &elem[1];
    }
    else if(elem[2] == '\0')
    {
      // end of options marker
      ++idx_;
      done_ = true;
    }
    else
    {
      // long option found
    }
  }
}

} // xes
