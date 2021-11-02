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

#ifndef CUTI_SIGNED_READER_HPP_
#define CUTI_SIGNED_READER_HPP_

#include "bound_inbuf.hpp"
#include "charclass.hpp"
#include "digits_reader.hpp"
#include "result.hpp"
#include "subroutine.hpp"

#include <limits>
#include <type_traits>
#include <utility>

namespace cuti
{

template<typename T>
struct signed_reader_t
{
  static_assert(std::is_signed_v<T>);
  static_assert(std::is_integral_v<T>);

  using value_t = T;

  signed_reader_t(result_t<T>& result, bound_inbuf_t& buf)
  : result_(result)
  , buf_(buf)
  , negative_()
  , digits_reader_(*this, &signed_reader_t::on_failure, buf_)
  { }

  void start()
  {
    negative_ = false;
    this->skip_spaces_and_check_minus();
  }

private :
  using UT = std::make_unsigned_t<T>;

  void skip_spaces_and_check_minus()
  {
    int c{};
    while(buf_.readable() && is_whitespace(c = buf_.peek()))
    {
      buf_.skip();
    }

    if(!buf_.readable())
    {
      buf_.call_when_readable([this] { this->skip_spaces_and_check_minus(); });
      return;
    }

    UT max = std::numeric_limits<T>::max();
    if(c == '-')
    {
      negative_ = true;
      ++max;
      buf_.skip();
    }

    digits_reader_.start(&signed_reader_t::on_digits_read, max);
  }

  void on_digits_read(UT unsigned_value)
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

  void on_failure(std::exception_ptr ex)
  {
    result_.fail(std::move(ex));
  }
    
private :
  result_t<T>& result_;
  bound_inbuf_t& buf_;
  bool negative_;
  subroutine_t<signed_reader_t, digits_reader_t<UT>> digits_reader_;
};

} // cuti

#endif
