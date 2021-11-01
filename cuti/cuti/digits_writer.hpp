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

#ifndef CUTI_DIGITS_WRITER_HPP_
#define CUTI_DIGITS_WRITER_HPP_

#include "bound_outbuf.hpp"
#include "result.hpp"

#include <limits>
#include <type_traits>

namespace cuti
{

template<typename T>
struct digits_writer_t
{
  static_assert(std::is_unsigned_v<T>);

  using value_t = void;

  digits_writer_t(result_t<void>& result, bound_outbuf_t& buf)
  : result_(result)
  , buf_(buf)
  , value_()
  , divisor_()
  { }

  digits_writer_t(digits_writer_t const&) = delete;
  digits_writer_t& operator=(digits_writer_t const&) = delete;
  
  void start(T value)
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

private :
  void write_digits()
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
      
private :
  result_t<void>& result_;
  bound_outbuf_t& buf_;
  T value_;
  T divisor_;
};

} // cuti

#endif
