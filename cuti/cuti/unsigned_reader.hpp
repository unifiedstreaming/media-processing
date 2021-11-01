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

#ifndef CUTI_UNSIGNED_READER_HPP_
#define CUTI_UNSIGNED_READER_HPP_

#include "bound_inbuf.hpp"
#include "charclass.hpp"
#include "digits_reader.hpp"
#include "result.hpp"
#include "subreader.hpp"

#include <limits>
#include <type_traits>
#include <utility>

namespace cuti
{

template<typename T>
struct unsigned_reader_t
{
  static_assert(std::is_unsigned_v<T>);

  using value_t = T;

  unsigned_reader_t(result_t<T>& result, bound_inbuf_t& buf)
  : result_(result)
  , buf_(buf)
  , digits_reader_(*this, &unsigned_reader_t::on_failure, buf)
  { }

  unsigned_reader_t(unsigned_reader_t const&) = delete;
  unsigned_reader_t& operator=(unsigned_reader_t const&) = delete;

  void start()
  {
    this->skip_spaces();
  }

private :
  void skip_spaces()
  {
    while(buf_.readable() && is_whitespace(buf_.peek()))
    {
      buf_.skip();
    }

    if(!buf_.readable())
    {
      buf_.call_when_readable([this] { this->skip_spaces(); });
      return;
    }

    digits_reader_.start(
      &unsigned_reader_t::on_digits_read, std::numeric_limits<T>::max());
  }    

  void on_digits_read(T value)
  {
    result_.submit(value);
  }

  void on_failure(std::exception_ptr ex)
  {
    result_.fail(std::move(ex));
  }

private :
  result_t<T>& result_;
  bound_inbuf_t& buf_;
  subreader_t<unsigned_reader_t, digits_reader_t<T>> digits_reader_;
};

} // cuti

#endif
