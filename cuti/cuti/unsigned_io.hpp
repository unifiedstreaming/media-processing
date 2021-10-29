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

#ifndef CUTI_UNSIGNED_IO_HPP_
#define CUTI_UNSIGNED_IO_HPP_

#include "bound_inbuf.hpp"
#include "charclass.hpp"
#include "parse_error.hpp"
#include "result.hpp"

#include <limits>
#include <type_traits>

namespace cuti
{

template<typename T>
struct unsigned_reader_t
{
  static_assert(std::is_unsigned_v<T>);

  unsigned_reader_t(result_t<T>& result, bound_inbuf_t& buf)
  : result_(result)
  , buf_(buf)
  { }

  unsigned_reader_t(unsigned_reader_t const&) = delete;
  unsigned_reader_t& operator=(unsigned_reader_t const&) = delete;

  void start()
  {
    digit_seen_ = false;
    value_ = 0;
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

    this->read_digits();
  }

  void read_digits()
  {
    static T constexpr max_value = std::numeric_limits<T>::max();

    int dval{};
    while(buf_.readable() && (dval = digit_value(buf_.peek())) >= 0)
    {
      digit_seen_ = true;

      T udval = static_cast<T>(dval);
      if(value_ > max_value / 10 || udval > max_value - 10 * value_)
      {
        result_.set_exception(parse_error_t("unsigned integral overflow"));
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
      result_.set_exception(parse_error_t("digit expected"));
      return;
    }
  
    result_.set_value(value_);
  }

private :
  result_t<T>& result_;
  bound_inbuf_t& buf_;
  bool digit_seen_;
  T value_;
};

} // cuti

#endif
