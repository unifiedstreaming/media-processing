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

#include "integral_readers.hpp"

#include "charclass.hpp"
#include "parse_error.hpp"

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

} // detail

} // cuti
