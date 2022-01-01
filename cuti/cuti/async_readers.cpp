/*
 * Copyright (C) 2021-2022 CodeShop B.V.
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

#include "async_readers.hpp"

#include "charclass.hpp"
#include "parse_error.hpp"
#include "remote_error.hpp"
#include "stack_marker.hpp"

#include <cassert>
#include <limits>

namespace cuti
{

namespace detail
{

struct whitespace_skipper_t::exception_handler_t
{
  exception_handler_t(any_result_t& result, bound_inbuf_t& buf)
  : result_(result)
  , error_reader_(*this, result_, buf)
  { }

  exception_handler_t(exception_handler_t const&) = delete;
  exception_handler_t& operator=(exception_handler_t const&) = delete;

  void start()
  {
    error_reader_.start(&exception_handler_t::on_error_read);
  }

private :
  void on_error_read(remote_error_t error)
  {
    result_.fail(std::move(error));
  }

private :
  any_result_t& result_;
  subroutine_t<exception_handler_t, reader_t<remote_error_t>> error_reader_;
};
  
void whitespace_skipper_t::exception_handler_deleter_t::operator()(
  exception_handler_t* handler) const noexcept
{
  delete handler;
}

void whitespace_skipper_t::start_exception_handler()
{
  assert(buf_.readable());
  assert(buf_.peek() == '!');
  buf_.skip();

  if(exception_handler_ == nullptr)
  {
    exception_handler_.reset(new exception_handler_t(result_, buf_));
  }

  exception_handler_->start();
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
  int c{};
  int dval{};
  while(buf_.readable() && (dval = digit_value(c = buf_.peek())) >= 0)
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

  if(c == eof)
  {
    // avoid submitting a half-baked value on remote hangup
    result_.fail(parse_error_t("unexpected eof in integral value"));
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
boolean_reader_t<T>::boolean_reader_t(result_t<T>& result, bound_inbuf_t& buf)
: result_(result)
, buf_(buf)
, skipper_(*this, result_, buf_)
{ }

template<typename T>
void boolean_reader_t<T>::start()
{
  skipper_.start(&boolean_reader_t::on_whitespace_skipped);
}

template<typename T>
void boolean_reader_t<T>::on_whitespace_skipped(int c)
{
  assert(buf_.readable());
  assert(buf_.peek() == c);

  T value{};
  switch(c)
  {
  case '&' :
    value = false;
    break;
  case '|' :
    value = true;
    break;
  default :
    result_.fail(parse_error_t("boolean value (\'&\' or \'|\') expected"));
    return;
  }
  buf_.skip();

  result_.submit(value);
}
  
template struct boolean_reader_t<bool>;
template struct boolean_reader_t<flag_t>;

template<typename T>
unsigned_reader_t<T>::unsigned_reader_t(result_t<T>& result,
                                        bound_inbuf_t& buf)
: result_(result)
, skipper_(*this, result_, buf)
, digits_reader_(*this, result_, buf)
{ }

template<typename T>
void unsigned_reader_t<T>::start()
{
  skipper_.start(&unsigned_reader_t::on_whitespace_skipped);
}

template<typename T>
void unsigned_reader_t<T>::on_whitespace_skipped(int)
{
  digits_reader_.start(
    &unsigned_reader_t::on_digits_read, std::numeric_limits<T>::max());
}    

template<typename T>
void unsigned_reader_t<T>::on_digits_read(T value)
{
  result_.submit(value);
}

template struct unsigned_reader_t<unsigned short>;
template struct unsigned_reader_t<unsigned int>;
template struct unsigned_reader_t<unsigned long>;
template struct unsigned_reader_t<unsigned long long>;

template<typename T>
signed_reader_t<T>::signed_reader_t(result_t<T>& result, bound_inbuf_t& buf)
: result_(result)
, buf_(buf)
, skipper_(*this, result_, buf_)
, digits_reader_(*this, result_, buf_)
, negative_()
{ }

template<typename T>
void signed_reader_t<T>::start()
{
  negative_ = false;
  skipper_.start(&signed_reader_t::on_whitespace_skipped);
}

template<typename T>
void signed_reader_t<T>::on_whitespace_skipped(int c)
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

template struct signed_reader_t<short>;
template struct signed_reader_t<int>;
template struct signed_reader_t<long>;
template struct signed_reader_t<long long>;

template<typename T>
blob_reader_t<T>::blob_reader_t(result_t<T>& result, bound_inbuf_t& buf)
: result_(result)
, buf_(buf)
, skipper_(*this, result_, buf_)
, hex_digits_reader_(*this, result_, buf_)
, value_()
{ }

template<typename T>
void blob_reader_t<T>::start()
{
  value_.clear();
  skipper_.start(&blob_reader_t::read_leading_dq);
}

template<typename T>
void blob_reader_t<T>::read_leading_dq(int c)
{
  assert(buf_.readable());
  assert(buf_.peek() == c);

  if(c != '\"')
  {
    result_.fail(parse_error_t("opening double quote ('\"') expected"));
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

template struct blob_reader_t<std::string>;
template struct blob_reader_t<std::vector<char>>;
template struct blob_reader_t<std::vector<signed char>>;
template struct blob_reader_t<std::vector<unsigned char>>;

identifier_reader_t::identifier_reader_t(result_t<identifier_t>& result,
                                         bound_inbuf_t& buf)
: result_(result)
, buf_(buf)
, skipper_(*this, result_, buf_)
, wrapped_()
{ }

void identifier_reader_t::start()
{
  wrapped_.clear();

  skipper_.start(&identifier_reader_t::read_leader);
}

void identifier_reader_t::read_leader(int c)
{
  assert(buf_.readable());
  assert(buf_.peek() == c);

  if(!identifier_t::is_leader(c))
  {
    result_.fail(parse_error_t("identifier expected"));
    return;
  }

  wrapped_.push_back(static_cast<char>(c));
  buf_.skip();

  this->read_followers();
}

void identifier_reader_t::read_followers()
{
  int c{};
  while(buf_.readable() && identifier_t::is_follower(c = buf_.peek()))
  {
    wrapped_.push_back(static_cast<char>(c));
    buf_.skip();
  }

  if(!buf_.readable())
  {
    buf_.call_when_readable([this] { this->read_followers(); });
    return;
  }

  if(c == eof)
  {
    // avoid submitting a half-baked value on remote hangup
    result_.fail(parse_error_t("unexpected eof in identifier value"));
    return;
  }

  result_.submit(std::move(wrapped_));
}

} // detail

} // cuti
