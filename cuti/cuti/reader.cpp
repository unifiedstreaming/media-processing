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

#include "reader.hpp"

#include "charclass.hpp"
#include "parse_error.hpp"

#include <cassert>
#include <limits>
#include <utility>

namespace cuti
{

namespace detail
{

token_finder_t::token_finder_t(result_t<int>& result,
                               bound_inbuf_t& buf)
: result_(result)
, buf_(buf)
{ }

void token_finder_t::start()
{
  int c{};
  while(buf_.readable() && is_whitespace(c = buf_.peek()))
  {
    buf_.skip();
  }

  if(!buf_.readable())
  {
    buf_.call_when_readable([this] { this->start(); });
    return;
  }

  // TODO: check for inline exception in buf_ and fail if so 

  result_.submit(c);
}
  
template<typename T>
digits_reader_t<T>::digits_reader_t(result_t<T>& result, bound_inbuf_t& buf)
: result_(result)
, buf_(buf)
, max_()
, digit_seen_()
, value_()
{ }

template<typename T>
void digits_reader_t<T>::start(T max)
{
  max_ = max;
  digit_seen_ = false;
  value_ = 0;

  this->read_digits();
}

template<typename T>
void digits_reader_t<T>::read_digits()
{
  int dval{};
  while(buf_.readable() && (dval = digit_value(buf_.peek())) >= 0)
  {
    digit_seen_ = true;

    T udval = static_cast<T>(dval);
    if(value_ > max_ / 10 || udval > max_ - 10 * value_)
    {
      result_.fail(parse_error_t("integer overflow"));
      return;
    }

    value_ *= 10;
    value_ += udval;

    buf_.skip();
  }

  if(!buf_.readable())
  {
    buf_.call_when_readable([this] { this->read_digits(); });
    return;
  }

  if(!digit_seen_)
  {
    result_.fail(parse_error_t("digit expected"));
    return;
  }
  
  result_.submit(value_);
}

template struct digits_reader_t<unsigned short>;
template struct digits_reader_t<unsigned int>;
template struct digits_reader_t<unsigned long>;
template struct digits_reader_t<unsigned long long>;

hex_digits_reader_t::hex_digits_reader_t(
  result_t<char>& result, bound_inbuf_t& buf)
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
    if(dval == -1)
    {
      result_.fail(parse_error_t("hex digit expected"));
      return;
    }

    shift_ -= 4;
    value_ |= static_cast<char>(dval << shift_);

    buf_.skip();
  }

  if(shift_ != 0)
  {
    buf_.call_when_readable([this] { this->read_digits(); });
    return;
  }

  result_.submit(value_);
}
  
template<typename T>
unsigned_reader_t<T>::unsigned_reader_t(result_t<T>& result,
                                        bound_inbuf_t& buf)
: result_(result)
, finder_(*this, &unsigned_reader_t::on_failure, buf)
, digits_reader_(*this, &unsigned_reader_t::on_failure, buf)
{ }

template<typename T>
void unsigned_reader_t<T>::start()
{
  finder_.start(&unsigned_reader_t::on_begin_token);
}

template<typename T>
void unsigned_reader_t<T>::on_begin_token(int)
{
  digits_reader_.start(
    &unsigned_reader_t::on_digits_read, std::numeric_limits<T>::max());
}    

template<typename T>
void unsigned_reader_t<T>::on_digits_read(T value)
{
  result_.submit(value);
}

template<typename T>
void unsigned_reader_t<T>::on_failure(std::exception_ptr ex)
{
  result_.fail(std::move(ex));
}

template struct unsigned_reader_t<unsigned short>;
template struct unsigned_reader_t<unsigned int>;
template struct unsigned_reader_t<unsigned long>;
template struct unsigned_reader_t<unsigned long long>;

template<typename T>
signed_reader_t<T>::signed_reader_t(result_t<T>& result, bound_inbuf_t& buf)
: result_(result)
, buf_(buf)
, finder_(*this, &signed_reader_t::on_failure, buf_)
, digits_reader_(*this, &signed_reader_t::on_failure, buf_)
, negative_()
{ }

template<typename T>
void signed_reader_t<T>::start()
{
  negative_ = false;
  finder_.start(&signed_reader_t::on_begin_token);
}

template<typename T>
void signed_reader_t<T>::on_begin_token(int c)
{
  assert(buf_.readable());
  assert(c == buf_.peek());

  UT max = std::numeric_limits<T>::max();
  if(c == '-')
  {
    negative_ = true;
    ++max;
    buf_.skip();
  }

  digits_reader_.start(&signed_reader_t::on_digits_read, max);
}

template<typename T>
void signed_reader_t<T>::on_digits_read(UT unsigned_value)
{
  T signed_value;

  if(!negative_ || unsigned_value == 0)
  {
    signed_value = unsigned_value;
  }
  else
  {
    --unsigned_value;
    signed_value = unsigned_value;
    signed_value = -signed_value;
    --signed_value;
  }

  result_.submit(signed_value);
}

template<typename T>
void signed_reader_t<T>::on_failure(std::exception_ptr ex)
{
  result_.fail(std::move(ex));
}
    
template struct signed_reader_t<short>;
template struct signed_reader_t<int>;
template struct signed_reader_t<long>;
template struct signed_reader_t<long long>;

string_reader_t::string_reader_t(
  result_t<std::string>& result, bound_inbuf_t& buf)
: result_(result)
, buf_(buf)
, finder_(*this, &string_reader_t::on_exception, buf_)
, hex_digits_reader_(*this, &string_reader_t::on_exception, buf_)
, value_()
, recursion_()
{ }

void string_reader_t::start()
{
  value_.clear();
  recursion_ = 0;

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
  ++recursion_;

  int c{};
  while(buf_.readable() && recursion_ != 100 && (c = buf_.peek()) != '\"')
  {
    switch(c)
    {
    case eof :
    case '\n' :
      result_.fail(parse_error_t("closing double quote (\'\"\') missing"));
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

  if(!buf_.readable() || recursion_ == 100)
  {
    recursion_ = 0;
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
  this->read_contents();
}

void string_reader_t::on_hex_digits(char c)
{
  value_ += c;
  this->read_contents();
}

void string_reader_t::on_exception(std::exception_ptr ex)
{
  result_.fail(std::move(ex));
}
  
} // detail

} // cuti
