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

#include "writer.hpp"

#include "charclass.hpp"

#include <cassert>
#include <utility>

namespace cuti
{

namespace detail
{

hex_digits_writer_t::hex_digits_writer_t(
  result_t<void>& result, bound_outbuf_t& buf)
: result_(result)
, buf_(buf)
, value_()
, shift_()
{ }

void hex_digits_writer_t::start(int value)
{
  value_ = value;
  shift_ = 8;

  this->write_digits();
}

void hex_digits_writer_t::write_digits()
{
  assert(shift_ % 4 == 0);

  while(shift_ != 0 && buf_.writable())
  {
    shift_ -= 4;
    buf_.put(hex_digits[(value_ >> shift_) & 0x0F]);
  }

  if(shift_ != 0)
  {
    buf_.call_when_writable([this] { this->write_digits(); });
    return;
  }

  result_.submit();
}

string_writer_t::string_writer_t(result_t<void>& result, bound_outbuf_t& buf)
: result_(result)
, buf_(buf)
, hex_digits_writer_(*this, &string_writer_t::on_exception, buf_)
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
    if(!is_printable(c) || c == '\"' || c == '\'' || c == '\\')
    {
      buf_.put('\\');
      this->write_escaped();
      return;
    }

    buf_.put(static_cast<char>(c));
    ++rp_;
  }

  if(rp_ != ep_)
  {
    recursion_ = 0;
    buf_.call_when_writable([this] { this->write_contents(); });
    return;
  }

  this->write_closing_dq();
}

void string_writer_t::write_escaped()
{
  if(!buf_.writable())
  {
    buf_.call_when_writable([this] { this->write_escaped(); });
    return;
  }
    
  assert(rp_ != ep_);
  int c = std::char_traits<char>::to_int_type(*rp_);
  ++rp_;

  switch(c)
  {
  case '\t' :
    buf_.put('t');
    break;
  case '\n' :
    buf_.put('n');
    break;
  case '\r' :
    buf_.put('r');
    break;
  case '\"' :
    buf_.put('\"');
    break;
  case '\'' :
    buf_.put('\'');
    break;
  case '\\' :
    buf_.put('\\');
    break;
  default :
    buf_.put('x');
    hex_digits_writer_.start(&string_writer_t::write_contents, c);
    return;
  }

  this->write_contents();
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

} // detail

} // cuti
