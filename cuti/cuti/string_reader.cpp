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

#include "string_reader.hpp"

#include "charclass.hpp"
#include "eof.hpp"
#include "parse_error.hpp"
#include "stack_marker.hpp"

namespace cuti
{

namespace detail
{

hex_digits_reader_t::hex_digits_reader_t(
  result_t<int>& result, bound_inbuf_t& buf)
: result_(result)
, buf_(buf)
, shift_()
, value_()
{ }

void hex_digits_reader_t::start()
{
  shift_ = 8;
  value_ = 0;

  this->read_digits();
}

void hex_digits_reader_t::read_digits()
{
  assert(shift_ % 4 == 0);

  while(shift_ != 0 && buf_.readable())
  {
    int dval = hex_digit_value(buf_.peek());
    if(dval < 0)
    {
      result_.fail(parse_error_t("hex digit expected"));
      return;
    }

    shift_ -= 4;
    value_ |= dval << shift_;

    buf_.skip();
  }

  if(shift_ != 0)
  {
    buf_.call_when_readable([this] { this->read_digits(); });
    return;
  }

  result_.submit(value_);
}
  
string_reader_t::string_reader_t(
  result_t<std::string>& result, bound_inbuf_t& buf)
: result_(result)
, buf_(buf)
, finder_(*this, &string_reader_t::on_exception, buf_)
, hex_digits_reader_(*this, &string_reader_t::on_exception, buf_)
, value_()
{ }

void string_reader_t::start()
{
  value_.clear();

  finder_.start(&string_reader_t::on_begin_token);
}

void string_reader_t::on_begin_token(int c)
{
  assert(buf_.readable());
  assert(buf_.peek() == c);

  if(c != '\"')
  {
    result_.fail(parse_error_t("opening double quote (\'\"\') expected"));
    return;
  }
  buf_.skip();

  this->read_contents();
}

void string_reader_t::read_contents()
{
  int c{};
  while(buf_.readable() && (c = buf_.peek()) != '\"')
  {
    switch(c)
    {
    case eof :
    case '\n' :
      result_.fail(parse_error_t(
        "missing closing double quote (\'\"\') at end of string"));
      return;
    case '\\' :
      buf_.skip();
      this->read_escaped();
      return;
    default :
      if(!is_printable(c))
      {
        result_.fail(parse_error_t("non-printable in string value"));
        return;
      }
      buf_.skip();
      value_ += static_cast<char>(c);
      break;
    }
  }

  if(!buf_.readable())
  {
    buf_.call_when_readable([this] { this->read_contents(); });
    return;
  }
  buf_.skip();

  result_.submit(std::move(value_));
}

void string_reader_t::read_escaped()
{
  if(!buf_.readable())
  {
    buf_.call_when_readable([this] { this->read_escaped(); });
    return;
  }

  switch(buf_.peek())
  {
  case 't' :
    value_ += '\t';
    break;
  case 'n' :
    value_ += '\n';
    break;
  case 'r' :
    value_ += '\r';
    break;
  case 'x' :
    buf_.skip();
    hex_digits_reader_.start(&string_reader_t::on_hex_digits);
    return;
  case '\"' :
    value_ += '\"';
    break;
  case '\'' :
    value_ += '\'';
    break;
  case '\\' :
    value_ += '\\';
    break;
  default :
    result_.fail(parse_error_t("unknown escape sequence in string value"));
    return;
  }

  buf_.skip();

  stack_marker_t marker;
  if(marker.in_range(buf_.base_marker()))
  {
    this->read_contents();
    return;
  }

  buf_.call_when_readable([this] { this->read_contents(); });
}

void string_reader_t::on_hex_digits(int c)
{
  value_ += static_cast<char>(c);

  stack_marker_t marker;
  if(marker.in_range(buf_.base_marker()))
  {
    this->read_contents();
    return;
  }

  buf_.call_when_readable([this] { this->read_contents(); });
}

void string_reader_t::on_exception(std::exception_ptr ex)
{
  result_.fail(std::move(ex));
}
  
} // detail

} // cuti
