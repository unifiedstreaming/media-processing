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

#include "integral_writers.hpp"

#include <cassert>
#include <limits>

namespace cuti
{

namespace detail
{

template<typename T>
digits_writer_t<T>::digits_writer_t(result_t<void>& result,
                                    bound_outbuf_t& buf)
: result_(result)
, buf_(buf)
, value_()
, divisor_()
{ }

template<typename T>
void digits_writer_t<T>::start(T value)
{
  static T constexpr max = std::numeric_limits<T>::max();

  value_ = value;
  divisor_ = 1;
  while(divisor_ <= max / 10 && divisor_ * 10 <= value_)
  {
    divisor_ *= 10;
  }

  this->write_digits();
}

template<typename T>
void digits_writer_t<T>::write_digits()
{
  while(divisor_ >= 1 && buf_.writable())
  {
    buf_.put(static_cast<char>((value_ / divisor_) + '0'));
    value_ %= divisor_;
    divisor_ /= 10;
  }

  if(divisor_ >= 1)
  {
    buf_.call_when_writable([this] { this->write_digits(); });
    return;
  }

  result_.submit();
}
      
template struct digits_writer_t<unsigned short>;
template struct digits_writer_t<unsigned int>;
template struct digits_writer_t<unsigned long>;
template struct digits_writer_t<unsigned long long>;

template<typename T>
unsigned_writer_t<T>::unsigned_writer_t(result_t<void>& result,
                                        bound_outbuf_t& buf)
: result_(result)
, buf_(buf)
, digits_writer_(*this, &unsigned_writer_t::on_failure, buf_)
, value_()
{ }

template<typename T>
void unsigned_writer_t<T>::start(T value)
{
  value_ = value;
  this->write_space();
}

template<typename T>
void unsigned_writer_t<T>::write_space()
{
  if(!buf_.writable())
  {
    buf_.call_when_writable([this] { this->write_space(); });
    return;
  }
    buf_.put(' ');

  digits_writer_.start(&unsigned_writer_t::on_digits_written, value_);
}

template<typename T>
void unsigned_writer_t<T>::on_digits_written()
{
  result_.submit();
}

template<typename T>
void unsigned_writer_t<T>::on_failure(std::exception_ptr ex)
{
  result_.fail(std::move(ex));
}

template struct unsigned_writer_t<unsigned short>;
template struct unsigned_writer_t<unsigned int>;
template struct unsigned_writer_t<unsigned long>;
template struct unsigned_writer_t<unsigned long long>;

template<typename T>
signed_writer_t<T>::signed_writer_t(result_t<void>& result, bound_outbuf_t& buf)
: result_(result)
, buf_(buf)
, digits_writer_(*this, &signed_writer_t::on_failure, buf_)
, value_()
{ }

template<typename T>
void signed_writer_t<T>::start(T value)
{
  value_ = value;
  this->write_space();
}

template<typename T>
void signed_writer_t<T>::write_space()
{
  if(!buf_.writable())
  {
    buf_.call_when_writable([this] { this->write_space(); });
    return;
  }
  buf_.put(' ');

  if(value_ < 0)
  {
    this->write_minus();
  }
  else
  {
    UT unsigned_value = value_;
    digits_writer_.start(
      &signed_writer_t::on_digits_written, unsigned_value);
  }
}

template<typename T>
void signed_writer_t<T>::write_minus()
{
  if(!buf_.writable())
  {
    buf_.call_when_writable([this] { this->write_minus(); });
    return;
  }
  buf_.put('-');
  
  assert(value_ < 0);
  T signed_value = value_ + 1;
  signed_value = -signed_value;
  UT unsigned_value = signed_value;
  ++unsigned_value;

  digits_writer_.start(
    &signed_writer_t::on_digits_written, unsigned_value);
}

template<typename T>
void signed_writer_t<T>::on_digits_written()
{
  result_.submit();
}

template<typename T>
void signed_writer_t<T>::on_failure(std::exception_ptr ex)
{
  result_.fail(std::move(ex));
}

template struct signed_writer_t<short>;
template struct signed_writer_t<int>;
template struct signed_writer_t<long>;
template struct signed_writer_t<long long>;

} // detail

} // cuti
