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

#include "reader_utils.hpp"

#include "charclass.hpp"
#include "parse_error.hpp"
#include "stack_marker.hpp"

#include <cassert>
#include <limits>

namespace cuti
{

namespace detail
{

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
      result_.fail(parse_error_t("integral type overflow"));
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
  
template<typename T>
blob_reader_t<T>::blob_reader_t(result_t<T>& result, bound_inbuf_t& buf)
: result_(result)
, buf_(buf)
, finder_(*this, &blob_reader_t::on_exception, buf_)
, hex_digits_reader_(*this, &blob_reader_t::on_exception, buf_)
, value_()
{ }

template<typename T>
void blob_reader_t<T>::start()
{
  value_.clear();

  finder_.start(&blob_reader_t::on_begin_token);
}

template<typename T>
void blob_reader_t<T>::on_begin_token(int c)
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

template<typename T>
void blob_reader_t<T>::read_contents()
{
  int c{};
  while(buf_.readable() && (c = buf_.peek()) != '\"')
  {
    switch(c)
    {
    case eof :
      result_.fail(parse_error_t("unexpected eof in string value"));
      return;
    case '\n' :
      result_.fail(parse_error_t("non-escaped newline string value"));
      return;
    case '\\' :
      buf_.skip();
      this->read_escaped();
      return;
    default :
      buf_.skip();
      value_.push_back(static_cast<typename T::value_type>(c));
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

template<typename T>
void blob_reader_t<T>::read_escaped()
{
  if(!buf_.readable())
  {
    buf_.call_when_readable([this] { this->read_escaped(); });
    return;
  }

  switch(buf_.peek())
  {
  case 't' :
    value_.push_back('\t');
    break;
  case 'n' :
    value_.push_back('\n');
    break;
  case 'r' :
    value_.push_back('\r');
    break;
  case 'x' :
    buf_.skip();
    hex_digits_reader_.start(&blob_reader_t::on_hex_digits);
    return;
  case '\"' :
    value_.push_back('\"');
    break;
  case '\'' :
    value_.push_back('\'');
    break;
  case '\\' :
    value_.push_back('\\');
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

template<typename T>
void blob_reader_t<T>::on_hex_digits(int c)
{
  value_.push_back(static_cast<typename T::value_type>(c));

  stack_marker_t marker;
  if(marker.in_range(buf_.base_marker()))
  {
    this->read_contents();
    return;
  }

  buf_.call_when_readable([this] { this->read_contents(); });
}

template<typename T>
void blob_reader_t<T>::on_exception(std::exception_ptr ex)
{
  result_.fail(std::move(ex));
}
  
template struct blob_reader_t<std::string>;
template struct blob_reader_t<std::vector<char>>;
template struct blob_reader_t<std::vector<signed char>>;
template struct blob_reader_t<std::vector<unsigned char>>;

} // detail

} // cuti
