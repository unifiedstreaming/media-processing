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

#ifndef CUTI_UNSIGNED_WRITER_HPP_
#define CUTI_UNSIGNED_WRITER_HPP_

#include "bound_outbuf.hpp"
#include "digits_writer.hpp"
#include "result.hpp"
#include "subroutine.hpp"

#include <type_traits>
#include <utility>

namespace cuti
{

template<typename T>
struct unsigned_writer_t
{
  static_assert(std::is_unsigned_v<T>);

  using value_t = void;

  unsigned_writer_t(result_t<void>& result, bound_outbuf_t& buf)
  : result_(result)
  , buf_(buf)
  , digits_writer_(*this, &unsigned_writer_t::on_failure, buf_)
  , value_()
  { }

  unsigned_writer_t(unsigned_writer_t const&);
  unsigned_writer_t& operator=(unsigned_writer_t const&);

  void start(T value)
  {
    value_ = value;
    this->write_space();
  }

private :
  void write_space()
  {
    if(!buf_.writable())
    {
      buf_.call_when_writable([this] { this->write_space(); });
      return;
    }

    buf_.put(' ');
    digits_writer_.start(&unsigned_writer_t::on_digits_written, value_);
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
  subroutine_t<unsigned_writer_t, digits_writer_t<T>> digits_writer_;
  T value_;
};

} // cuti

#endif
