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

inline auto constexpr skip_whitespace = skip_whitespace_t{};

inline int digit_value(int c)
{
  if(c < '0' || c > '9')
  {
    return -1;
  }

  return c - '0';
}

template<typename T>
struct read_first_digit_t
{
  static_assert(std::is_unsigned_v<T>);

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

    int dval = digit_value(source.peek());
    if(dval == -1)
    {
      cont.fail(std::make_exception_ptr(parse_error_t("digit expected")));
      return;
    }

    source.skip();
    cont.submit(source, T(dval), std::forward<Args>(args)...);
  }
};

template<typename T>
auto constexpr read_first_digit = read_first_digit_t<T>{};

template<typename T>
struct read_trailing_digits_t
{
  static_assert(std::is_unsigned_v<T>);

  template<typename Cont, typename... Args>
  void operator()(
    Cont&& cont, async_source_t source, T total, T limit, Args&&... args) const
  {
    int dval;
    while(source.readable() &&
          (dval = digit_value(source.peek())) != -1)
    {
      T udval = T(dval);
      if(total > limit / 10 || udval > limit - 10 * total)
      {
        cont.fail(std::make_exception_ptr(parse_error_t("integral overflow")));
        return;
      }

      total *= 10;
      total += udval;

      source.skip();
    }
      
    if(!source.readable())
    {
      source.call_when_readable(callback_t(*this, std::forward<Cont>(cont),
        source, total, limit, std::forward<Args>(args)...));
      return;
    }

    cont.submit(source, total, std::forward<Args>(args)...);
  }
};

template<typename T>
auto constexpr read_trailing_digits = read_trailing_digits_t<T>{};

template<typename T>
struct read_unsigned_t
{
  static_assert(std::is_unsigned_v<T>);
  static auto constexpr limit = std::numeric_limits<T>::max();

  template<typename Cont, typename... Args>
  void operator()(
    Cont&& cont, async_source_t source, Args&&... args) const
  {
    static constexpr auto chain = async_stitch(
      skip_whitespace, read_first_digit<T>, read_trailing_digits<T>);
    chain(std::forward<Cont>(cont), source, limit,
      std::forward<Args>(args)...);
  }
};
    
template<typename T>
auto constexpr read_unsigned = read_unsigned_t<T>{};

struct read_optional_sign_t
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

    bool negative = false;
    switch(source.peek())
    {
    case '-' :
      negative = true;
      source.skip();
      break;
    case '+' :
      source.skip();
      break;
    default :
      break;
    }

    cont.submit(source, negative, std::forward<Args>(args)...);
  }
};
      
inline auto constexpr read_optional_sign = read_optional_sign_t{};

template<typename T>
struct insert_limit_t
{
  static_assert(std::is_integral_v<T>);
  static_assert(std::is_signed_v<T>);
  
  template<typename Cont, typename... Args>
  void operator()(
    Cont&& cont, async_source_t source, bool negative, Args&&... args) const
  {
    std::make_unsigned_t<T> limit = std::numeric_limits<T>::max();
    if(negative)
    {
      ++limit;
    }
    cont.submit(source, limit, negative, std::forward<Args>(args)...);
  }
};
    
template<typename T>
auto constexpr insert_limit = insert_limit_t<T>{};

template<typename T>
struct apply_sign_t
{
  static_assert(std::is_unsigned_v<T>);
  
  template<typename Cont, typename... Args>
  void operator()(
    Cont&& cont, async_source_t source, T value, bool negative,
    Args&&... args) const
  {
    std::make_signed_t<T> signed_value;
    if(!negative)
    {
      signed_value = value;
    }
    else
    {
      --value;
      signed_value = value;
      signed_value = -signed_value;
      --signed_value;
    }
    cont.submit(source, signed_value, std::forward<Args>(args)...);
  }
};
    
template<typename T>
auto constexpr apply_sign = apply_sign_t<T>{};

template<typename T>
struct read_signed_t
{
  static_assert(std::is_signed_v<T>);
  static_assert(std::is_integral_v<T>);

  template<typename Cont, typename... Args>
  void operator()(
    Cont&& cont, async_source_t source, Args&&... args) const
  {
    using UT = std::make_unsigned_t<T>;
    static constexpr auto chain = async_stitch(
      skip_whitespace, read_optional_sign, insert_limit<T>,
      read_first_digit<UT>, read_trailing_digits<UT>, apply_sign<UT>);
    chain(std::forward<Cont>(cont), source, std::forward<Args>(args)...);
  }
};
    
template<typename T>
auto constexpr read_signed = read_signed_t<T>{};

} // namespace cuti::detail

inline auto constexpr drop_source = detail::drop_source_t{};
inline auto constexpr check_eof = detail::check_eof_t{};

} // namespace cuti

#endif
