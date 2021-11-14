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

#include "writer_utils.hpp"

#include <limits>

namespace cuti
{

namespace detail
{

literal_writer_t::literal_writer_t(result_t<void>& result, bound_outbuf_t& buf)
: result_(result)
, buf_(buf)
{ }

void literal_writer_t::write_chars()
{
  while(first_ != last_ && buf_.writable())
  {
    buf_.put(*first_);
    ++first_;
  }

  if(first_ != last_)
  {
    buf_.call_when_writable([this] { this->write_chars(); });
    return;
  }

  result_.submit();
}

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

} // detail

} // cuti
