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

#include "string_writer.hpp"

#include "charclass.hpp"

#include <cassert>
#include <utility>

namespace cuti
{

string_writer_t::char_escape_writer_t::char_escape_writer_t(
  result_t<void>& result, bound_outbuf_t& buf)
: result_(result)
, buf_(buf)
, value_()
{ }

void string_writer_t::char_escape_writer_t::start(char value)
{
  value_ = value;

  this->write_backslash();
}

void string_writer_t::char_escape_writer_t::write_backslash()
{
  if(!buf_.writable())
  {
    buf_.call_when_writable([this] { this->write_backslash(); });
    return;
  }
  buf_.put('\\');

  this->write_value();
}

void string_writer_t::char_escape_writer_t::write_value()
{
  if(!buf_.writable())
  {
    buf_.call_when_writable([this] { this->write_value(); });
    return;
  }
  buf_.put(value_);

  result_.submit();
}

string_writer_t::hex_escape_writer_t::hex_escape_writer_t(
  result_t<void>& result, bound_outbuf_t& buf)
: result_(result)
, buf_(buf)
, value_()
, shift_()
{ }

void string_writer_t::hex_escape_writer_t::start(int value)
{
  value_ = value;
  shift_ = 8;

  this->write_backslash();
}

void string_writer_t::hex_escape_writer_t::write_backslash()
{
  if(!buf_.writable())
  {
    buf_.call_when_writable([this] { this->write_backslash(); });
    return;
  }
  buf_.put('\\');

  this->write_x();
}

void string_writer_t::hex_escape_writer_t::write_x()
{
  if(!buf_.writable())
  {
    buf_.call_when_writable([this] { this->write_x(); });
    return;
  }
  buf_.put('x');

  this->write_hex_digits();
}

void string_writer_t::hex_escape_writer_t::write_hex_digits()
{
  assert(shift_ % 4 == 0);

  while(shift_ != 0 && buf_.writable())
  {
    shift_ -= 4;
    buf_.put(hex_digits[(value_ >> shift_) & 0x0F]);
  }

  if(shift_ != 0)
  {
    buf_.call_when_writable([this] { this->write_hex_digits(); });
    return;
  }

  result_.submit();
}

string_writer_t::string_writer_t(result_t<void>& result, bound_outbuf_t& buf)
: result_(result)
, buf_(buf)
, char_escape_writer_(*this, &string_writer_t::on_exception, buf_)
, hex_escape_writer_(*this, &string_writer_t::on_exception, buf_)
, value_()
, rp_()
, ep_()
, recursion_(0)
{ }

void string_writer_t::start(std::string value)
{
  value_ = std::move(value);
  rp_ = value_.data();
  ep_ = rp_ + value_.size();
  recursion_ = 0;
    
  this->write_space();
}

void string_writer_t::write_space()
{
  if(!buf_.writable())
  {
    buf_.call_when_writable([this] { this->write_space(); });
    return;
  }
  buf_.put(' ');

  this->write_opening_dq();
}

void string_writer_t::write_opening_dq()
{
  if(!buf_.writable())
  {
    buf_.call_when_writable([this] { this->write_opening_dq(); });
    return;
  }
  buf_.put('\"');

  this->write_contents();
}

void string_writer_t::write_contents()
{
  ++recursion_;

  while(rp_ != ep_ && buf_.writable() && recursion_ != 100)
  {
    int c = std::char_traits<char>::to_int_type(*rp_);
    ++rp_;

    switch(c)
    {
    case '\t' :
      char_escape_writer_.start(&string_writer_t::write_contents, 't');
      return;
    case '\n' :
      char_escape_writer_.start(&string_writer_t::write_contents, 'n');
      return;
    case '\r' :
      char_escape_writer_.start(&string_writer_t::write_contents, 'r');
      return;
    case '\"' :
      char_escape_writer_.start(&string_writer_t::write_contents, '\"');
      return;
    case '\'' :
      char_escape_writer_.start(&string_writer_t::write_contents, '\'');
      return;
    case '\\' :
      char_escape_writer_.start(&string_writer_t::write_contents, '\\');
      return;
    default :
      if(!is_printable(c))
      {
        hex_escape_writer_.start(&string_writer_t::write_contents, c);
        return;
      }
      buf_.put(static_cast<char>(c));
      break;
    }
  }

  if(rp_ != ep_)
  {
    recursion_ = 0;
    buf_.call_when_writable([this] { this->write_contents(); });
    return;
  }

  this->write_closing_dq();
}
  
void string_writer_t::write_closing_dq()
{
  if(!buf_.writable())
  {
    buf_.call_when_writable([this] { this->write_closing_dq(); });
    return;
  }
  buf_.put('\"');

  result_.submit();
}

void string_writer_t::on_exception(std::exception_ptr ex)
{
  result_.fail(std::move(ex));
}

} // cuti
