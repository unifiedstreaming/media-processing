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

#ifndef CUTI_SIGNED_WRITER_HPP_
#define CUTI_SIGNED_WRITER_HPP_

#include "bound_outbuf.hpp"
#include "digits_writer.hpp"
#include "result.hpp"
#include "subroutine.hpp"

#include <cassert>
#include <type_traits>
#include <utility>

namespace cuti
{

template<typename T>
struct signed_writer_t
{
  static_assert(std::is_signed_v<T>);
  static_assert(std::is_integral_v<T>);

  using value_t = void;

  signed_writer_t(result_t<void>& result, bound_outbuf_t& buf)
  : result_(result)
  , buf_(buf)
  , digits_writer_(*this, &signed_writer_t::on_failure, buf_)
  , value_()
  { }

  void start(T value)
  {
    value_ = value;
    this->write_space();
  }

private :
  using UT = std::make_unsigned_t<T>;

  void write_space()
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

  void write_minus()
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

  void on_digits_written()
  {
    result_.submit();
  }

  void on_failure(std::exception_ptr ex)
  {
    result_.fail(std::move(ex));
  }

private :
  result_t<void>& result_;
  bound_outbuf_t& buf_;
  subroutine_t<signed_writer_t, digits_writer_t<UT>> digits_writer_;
  T value_;
};

} // cuti

#endif
