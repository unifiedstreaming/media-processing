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
#include <string>
#include <vector>
#include <type_traits>

namespace cuti
{

namespace detail
{

struct not_supported_t { };

} // namespace cuti::detail

template<typename T>
auto constexpr async_read = detail::not_supported_t{};

namespace detail
{

struct drop_source_t
{
  template<typename Cont, typename... Args>
  void operator()(Cont cont, async_source_t /* source */, Args&&... args) const
  {
    cont.submit(std::forward<Args>(args)...);
  }
};

struct read_eof_t
{
  template<typename Cont, typename... Args>
  void operator()(Cont cont, async_source_t source, Args&&... args) const
  {
    if(!source.readable())
    {
      source.call_when_readable(*this,
        cont, source, std::forward<Args>(args)...);
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
  void operator()(Cont cont, async_source_t source, Args&&... args) const
  {
    while(source.readable() && is_whitespace(source.peek()))
    {
      source.skip();
    }

    if(!source.readable())
    {
      source.call_when_readable(*this,
        cont, source, std::forward<Args>(args)...);
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
  void operator()(Cont cont, async_source_t source, Args&&... args) const
  {
    if(!source.readable())
    {
      source.call_when_readable(*this,
        cont, source, std::forward<Args>(args)...);
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
  void operator()(Cont cont, async_source_t source, T total, T limit,
                  Args&&... args) const
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
      source.call_when_readable(*this,
        cont, source, total, limit, std::forward<Args>(args)...);
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
  void operator()(Cont cont, async_source_t source, Args&&... args) const
  {
    static constexpr auto chain = async_stitch(
      skip_whitespace, read_first_digit<T>, read_trailing_digits<T>);
    chain(cont, source, limit, std::forward<Args>(args)...);
  }
};
    
template<typename T>
auto constexpr read_unsigned = read_unsigned_t<T>{};

struct read_optional_sign_t
{
  template<typename Cont, typename... Args>
  void operator()(Cont cont, async_source_t source, Args&&... args) const
  {
    if(!source.readable())
    {
      source.call_when_readable(*this,
        cont, source, std::forward<Args>(args)...);
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
  void operator()(Cont cont, async_source_t source, bool negative,
                  Args&&... args) const
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
  void operator()(Cont cont, async_source_t source, T value, bool negative,
                  Args&&... args) const
  {
    std::make_signed_t<T> signed_value;
    if(value == 0 || !negative)
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
  void operator()(Cont cont, async_source_t source, Args&&... args) const
  {
    using UT = std::make_unsigned_t<T>;
    static constexpr auto chain = async_stitch(
      skip_whitespace, read_optional_sign, insert_limit<T>,
      read_first_digit<UT>, read_trailing_digits<UT>, apply_sign<UT>);
    chain(cont, source, std::forward<Args>(args)...);
  }
};
    
template<typename T>
auto constexpr read_signed = read_signed_t<T>{};

struct read_double_quote_t
{
  template<typename Cont, typename... Args>
  void operator()(Cont cont, async_source_t source, Args&&... args) const
  {
    if(!source.readable())
    {
      source.call_when_readable(*this,
        cont, source, std::forward<Args>(args)...);
      return;
    }

    if(source.peek() != '"')
    {
      cont.fail(std::make_exception_ptr(parse_error_t("'\"' expected")));
      return;
    }

    source.skip();
    cont.submit(source, std::forward<Args>(args)...);
  }
};

inline auto constexpr read_double_quote = read_double_quote_t{};

inline int hex_digit_value(int c)
{
  int result;

  if(c >= '0' && c <= '9')
  {
    result = c - '0';
  }
  else if(c >= 'A' && c <= 'F')
  {
    result = c - 'A' + 10;
  }
  else if(c >= 'a' && c <= 'f')
  {
    result = c - 'a' + 10;
  }
  else
  {
    result = -1;
  }
  
  return result;
}

struct append_hex_digits_t
{
  template<typename Cont, typename... Args>
  void operator()(Cont cont, async_source_t source,
                  unsigned int count, unsigned int total, std::string&& value,
                  Args&&... args) const
  {
    while(count != 0)
    {
      if(!source.readable())
      {
        source.call_when_readable(*this,
          cont, source, count, total, std::move(value),
          std::forward<Args>(args)...);
        return;
      }

      int dval = hex_digit_value(source.peek());
      if(dval == -1)
      {
        cont.fail(std::make_exception_ptr(parse_error_t(
          "hex digit expected")));
        return;
      }

      source.skip();
      total *= 16;
      total += dval;

      --count;
    }

    value += static_cast<char>(total);
    cont.submit(source, std::move(value), std::forward<Args>(args)...);
  }
};

inline auto constexpr append_hex_digits = append_hex_digits_t{};

struct append_string_escape_t
{
  template<typename Cont, typename... Args>
  void operator()(Cont cont, async_source_t source, std::string&& value,
                  Args&&... args) const
  {
    if(!source.readable())
    {
      source.call_when_readable(*this,
        cont, source, std::move(value), std::forward<Args>(args)...);
      return;
    }

    switch(source.peek())
    {
    case '0' :
      value += '\0'; break;
    case 't' :
      value += '\t'; break;
    case 'n' :
      value += '\n'; break;
    case 'r' :
      value += '\r'; break;
    case '\\' :
      value += '\\'; break;
    case '"' :
      value += '"'; break;
    case 'x' :
      {
        source.skip();
        append_hex_digits(cont, source, 2, 0, std::move(value),
          std::forward<Args>(args)...);
        return;
      }
    default :
      {
        cont.fail(std::make_exception_ptr(parse_error_t(
          "illegal escape sequence in string value")));
        return;
      }
    }
        
    source.skip();
    cont.submit(source, std::move(value), std::forward<Args>(args)...);
  }
};    
          
inline auto constexpr append_string_escape = append_string_escape_t{};

inline bool is_literal_char(int c)
{
  switch(c)
  {
  case '"' :
  case '\\' :
    return false;
  default :
    return c >= 0x20 && c <= 0xFF;
  }
}

struct append_string_chars_t
{
  static auto constexpr max_recursion = 100;

  template<typename Cont, typename... Args>
  void operator()(Cont cont, async_source_t source,
                  std::string&& value, int recursion,
                  Args&&... args) const
  {
    int c;
    while(source.readable() && recursion != max_recursion &&
          is_literal_char(c = source.peek()))
    {
      value += static_cast<char>(c);
      source.skip();
    }

    if(!source.readable() || recursion == max_recursion)
    {
      source.call_when_readable(*this,
        cont, source, std::move(value), 0, std::forward<Args>(args)...);
      return;
    }

    if(c == '\\')
    {
      source.skip();
      static auto constexpr chain = async_stitch(
        append_string_escape, append_string_chars_t{});
      chain(cont, source, std::move(value), recursion + 1,
        std::forward<Args>(args)...);
      return;
    }

    if(c == '\n' || c == eof)
    {
      cont.fail(std::make_exception_ptr(parse_error_t(
        "missing terminating '\"'")));
      return;
    }

    if(c != '"')
    {
      cont.fail(std::make_exception_ptr(parse_error_t(
        "illegal character " + std::to_string(c) + " in string value")));
      return;
    }

    source.skip();
    cont.submit(source, std::move(value), std::forward<Args>(args)...);
  }
};

inline auto constexpr append_string_chars = append_string_chars_t{};

struct read_string_t
{
  template<typename Cont, typename... Args>
  void operator()(Cont cont, async_source_t source, Args&&... args) const
  {
    static auto constexpr chain = async_stitch(
      skip_whitespace, read_double_quote, append_string_chars);
    chain(cont, source, std::string{}, 0, std::forward<Args>(args)...);
  }
};
      
inline auto constexpr read_string = read_string_t{};

struct read_begin_sequence_t
{
  template<typename Cont, typename... Args>
  void operator()(Cont cont, async_source_t source, Args&&... args) const
  {
    if(!source.readable())
    {
      source.call_when_readable(*this,
        cont, source, std::forward<Args>(args)...);
      return;
    }

    if(source.peek() != '[')
    {
      cont.fail(std::make_exception_ptr(parse_error_t("'[' expected")));
      return;
    }

    source.skip();
    cont.submit(source, std::forward<Args>(args)...);
  }
};

inline auto constexpr read_begin_sequence = read_begin_sequence_t{};

template<typename T>
struct append_element_t
{
  template<typename Cont, typename TT, typename... Args>
  void operator()(Cont cont, async_source_t source,
                  TT&& element, std::vector<T>&& sequence,
                  Args&&... args) const
  {
    sequence.push_back(std::forward<TT>(element));
    cont.submit(source, std::move(sequence), std::forward<Args>(args)...);
  }
};

template<typename T>
auto constexpr append_element = append_element_t<T>{};

template<typename T>
struct append_sequence_t
{
  static auto constexpr max_recursion = 100;

  template<typename Cont, typename... Args>
  void operator()(Cont cont, async_source_t source,
                  std::vector<T>&& sequence, int recursion,
                  Args&&... args) const
  {
    if(!source.readable() || recursion == max_recursion)
    {
      source.call_when_readable(*this,
        cont, source, std::move(sequence), 0, std::forward<Args>(args)...);
      return;
    }

    if(source.peek() != ']')
    {
      static auto constexpr chain = async_stitch(
        async_read<T>, append_element<T>,
        skip_whitespace, append_sequence_t<T>{});
      chain(cont, source, std::move(sequence), recursion + 1,
        std::forward<Args>(args)...);
      return;
    }

    source.skip();
    cont.submit(source, std::move(sequence), std::forward<Args>(args)...);
 }
};
      
template<typename T>
auto constexpr append_sequence = append_sequence_t<T>{};

template<typename T>
struct read_sequence_t
{
  template<typename Cont, typename... Args>
  void operator()(Cont cont, async_source_t source, Args&&... args) const
  {
    static auto constexpr chain = async_stitch(skip_whitespace,
      read_begin_sequence, skip_whitespace, append_sequence<T>);
    chain(cont, source, std::vector<T>{}, 0, std::forward<Args>(args)...);
  }
};

template<typename T>
auto constexpr read_sequence = read_sequence_t<T>{};

} // namespace cuti::detail

inline auto constexpr drop_source = detail::drop_source_t{};
inline auto constexpr read_eof = detail::read_eof_t{};

template<>
inline auto constexpr async_read<unsigned short> =
  detail::read_unsigned<unsigned short>;

template<>
inline auto constexpr async_read<unsigned int> =
  detail::read_unsigned<unsigned int>;

template<>
inline auto constexpr async_read<unsigned long> =
  detail::read_unsigned<unsigned long>;

template<>
inline auto constexpr async_read<unsigned long long> =
  detail::read_unsigned<unsigned long long>;

template<>
inline auto constexpr async_read<short> =
  detail::read_signed<short>;

template<>
inline auto constexpr async_read<int> =
  detail::read_signed<int>;

template<>
inline auto constexpr async_read<long> =
  detail::read_signed<long>;

template<>
inline auto constexpr async_read<long long> =
  detail::read_signed<long long>;

template<>
inline auto constexpr async_read<std::string> =
  detail::read_string;

template<typename T>
auto constexpr async_read<std::vector<T>> =
  detail::read_sequence<T>;

} // namespace cuti

#endif
