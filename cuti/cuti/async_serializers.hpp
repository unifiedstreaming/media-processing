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

#ifndef CUTI_ASYNC_SERIALIZERS_HPP_
#define CUTI_ASYNC_SERIALIZERS_HPP_

#include "async_source.hpp"
#include "async_stitch.hpp"
#include "parse_error.hpp"

#include <exception>
#include <limits>
#include <type_traits>

namespace cuti
{

namespace detail
{

struct drop_source_t
{
  template<typename Cont, typename... Args>
  void operator()(
    Cont&& cont, async_source_t /* source */, Args&&... args) const
  {
    cont.submit(std::forward<Args>(args)...);
  }
};

struct check_eof_t
{
  template<typename Cont, typename... Args>
  void operator()(
    Cont&& cont, async_source_t source, Args&&... args) const
  {
    if(!source.readable())
    {
      source.call_when_readable(callback_t(*this, std::forward<Cont>(cont),
        source, std::forward<Args>(args)...));
      return;
    }

    if(source.peek() != eof)
    {
      cont.fail(std::make_exception_ptr(parse_error_t("eof expected")));
      return;
    }

    cont.submit(source, std::forward<Args>(args)...);
  }
};

inline bool is_whitespace(int c)
{
  return c == '\t' || c == '\r' || c == ' ';
}

struct skip_whitespace_t
{
  template<typename Cont, typename... Args>
  void operator()(
    Cont&& cont, async_source_t source, Args&&... args) const
  {
    while(source.readable() && is_whitespace(source.peek()))
    {
      source.skip();
    }

    if(!source.readable())
    {
      source.call_when_readable(callback_t(*this, std::forward<Cont>(cont),
        source, std::forward<Args>(args)...));
      return;
    }      
    
    cont.submit(source, std::forward<Args>(args)...);
  }
};

inline int digit_value(int c)
{
  if(c < '0' || c > '9')
  {
    return -1;
  }

  return c - '0';
}

struct read_first_digit_t
{
  template<typename Cont, typename... Args>
  void operator()(
    Cont&& cont, async_source_t source, Args&&... args) const
  {
    if(!source.readable())
    {
      source.call_when_readable(callback_t(*this, std::forward<Cont>(cont),
        source, std::forward<Args>(args)...));
      return;
    }

    int value = digit_value(source.peek());
    if(value == -1)
    {
      cont.fail(std::make_exception_ptr(parse_error_t("digit expected")));
      return;
    }

    source.skip();
    cont.submit(source, static_cast<unsigned char>(value),
      std::forward<Args>(args)...);
  }
};

template<typename T>
struct read_unsigned_trailing_digits_t
{
  static_assert(std::is_unsigned_v<T>);
  static constexpr T max = std::numeric_limits<T>::max();

  template<typename Cont, typename... Args>
  void operator()(
    Cont&& cont, async_source_t source, T total, Args&&... args) const
  {
    int dval;
    while(source.readable() &&
          (dval = digit_value(source.peek())) != -1)
    {
      unsigned char udval = static_cast<unsigned char>(dval);
      if(total > max / 10 || udval > max - 10 * total)
      {
        cont.fail(std::make_exception_ptr(
	  parse_error_t("unsigned integral overflow")));
        return;
      }

      total *= 10;
      total += udval;

      source.skip();
    }
      
    if(!source.readable())
    {
      source.call_when_readable(callback_t(*this, std::forward<Cont>(cont),
        source, total, std::forward<Args>(args)...));
      return;
    }

    cont.submit(source, total, std::forward<Args>(args)...);
  }
};
  
} // namespace cuti::detail

inline auto constexpr drop_source = detail::drop_source_t{};
inline auto constexpr check_eof = detail::check_eof_t{};
inline auto constexpr skip_whitespace = detail::skip_whitespace_t{};
inline auto constexpr read_first_digit = detail::read_first_digit_t{};

template<typename T>
auto constexpr read_unsigned_trailing_digits =
  detail::read_unsigned_trailing_digits_t<T>{};

template<typename T>
auto constexpr read_unsigned = async_stitch(
  skip_whitespace, read_first_digit, read_unsigned_trailing_digits<T>);

} // namespace cuti

#endif
