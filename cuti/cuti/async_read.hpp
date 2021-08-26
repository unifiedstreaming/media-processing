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

#ifndef CUTI_ASYNC_READ_HPP_
#define CUTI_ASYNC_READ_HPP_

#include "async_source.hpp"
#include "async_stitch.hpp"
#include "construct.hpp"
#include "parse_error.hpp"
#include "ref_args.hpp"

#include <exception>
#include <limits>
#include <optional>
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
  template<typename Next, typename... Args>
  void operator()(Next next, async_source_t /* source */, Args&&... args) const
  {
    next.submit(std::forward<Args>(args)...);
  }
};

struct read_eof_t
{
  template<typename Next, typename... Args>
  void operator()(Next next, async_source_t source, Args&&... args) const
  {
    if(!source.readable())
    {
      source.call_when_readable(*this,
        next, source, std::forward<Args>(args)...);
      return;
    }

    if(source.peek() != eof)
    {
      next.fail(parse_error_t("eof expected"));
      return;
    }

    next.submit(source, std::forward<Args>(args)...);
  }
};

template<char fixed>
struct read_fixed_char_t
{
  template<typename Next, typename... Args>
  void operator()(Next next, async_source_t source, Args&&... args) const
  {
    static int constexpr fixed_int =
      std::char_traits<char>::to_int_type(fixed);

    if(!source.readable())
    {
      source.call_when_readable(*this,
        next, source, std::forward<Args>(args)...);
      return;
    }

    if(source.peek() != fixed_int)
    {
      static char const single_quote[] = "\'";

      auto message = std::string(single_quote) +
        fixed + single_quote + " expected";

      next.fail(parse_error_t(message));
      return;
    }

    source.skip();
    next.submit(source, std::forward<Args>(args)...);
  }
};

template<char fixed>
auto constexpr read_fixed_char = read_fixed_char_t<fixed>{};
      
inline bool is_whitespace(int c)
{
  return c == '\t' || c == '\r' || c == ' ';
}

struct skip_whitespace_t
{
  template<typename Next, typename... Args>
  void operator()(Next next, async_source_t source, Args&&... args) const
  {
    while(source.readable() && is_whitespace(source.peek()))
    {
      source.skip();
    }

    if(!source.readable())
    {
      source.call_when_readable(*this,
        next, source, std::forward<Args>(args)...);
      return;
    }      
    
    next.submit(source, std::forward<Args>(args)...);
  }
};

inline auto constexpr skip_whitespace = skip_whitespace_t{};

struct read_bool_char_t
{
  template<typename Next, typename... Args>
  void operator()(Next next, async_source_t source, Args&&... args) const
  {
    if(!source.readable())
    {
      source.call_when_readable(*this,
        next, source, std::forward<Args>(args)...);
      return;
    }

    bool value = false;
    switch(source.peek())
    {
    case '~' :
      break;
    case '*' :
      value = true;
      break;
    default :
      next.fail(parse_error_t("boolean value ('~' or '*') expected"));
      return;
    }

    source.skip();
    next.submit(source, value, std::forward<Args>(args)...);
  }
};

inline auto constexpr read_bool_char = read_bool_char_t{};

inline auto constexpr read_bool = async_stitch(
  skip_whitespace,
  read_bool_char);

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

  template<typename Next, typename... Args>
  void operator()(Next next, async_source_t source, Args&&... args) const
  {
    if(!source.readable())
    {
      source.call_when_readable(*this,
        next, source, std::forward<Args>(args)...);
      return;
    }

    int dval = digit_value(source.peek());
    if(dval == -1)
    {
      next.fail(parse_error_t("digit expected"));
      return;
    }

    source.skip();
    next.submit(source, T(dval), std::forward<Args>(args)...);
  }
};

template<typename T>
auto constexpr read_first_digit = read_first_digit_t<T>{};

template<typename T, T limit>
struct read_trailing_digits_t
{
  static_assert(std::is_unsigned_v<T>);

  template<typename Next, typename... Args>
  void operator()(Next next, async_source_t source, T total,
                  Args&&... args) const
  {
    int dval;
    while(source.readable() &&
          (dval = digit_value(source.peek())) != -1)
    {
      T udval = T(dval);
      if(total > limit / 10 || udval > limit - 10 * total)
      {
        next.fail(parse_error_t("integral overflow"));
        return;
      }

      total *= 10;
      total += udval;

      source.skip();
    }
      
    if(!source.readable())
    {
      source.call_when_readable(*this,
        next, source, total, std::forward<Args>(args)...);
      return;
    }

    next.submit(source, total, std::forward<Args>(args)...);
  }
};

template<typename T, T limit>
auto constexpr read_trailing_digits = read_trailing_digits_t<T, limit>{};

template<typename T, T limit>
auto constexpr read_unsigned_digits = async_stitch(
  read_first_digit<T>,
  read_trailing_digits<T, limit>);

template<typename T>
auto constexpr read_unsigned = async_stitch(
  skip_whitespace,
  read_unsigned_digits<T, std::numeric_limits<T>::max()>);

enum class sign_t { positive, negative };

struct read_optional_sign_t
{
  template<typename Next, typename... Args>
  void operator()(Next next, async_source_t source, Args&&... args) const
  {
    if(!source.readable())
    {
      source.call_when_readable(*this,
        next, source, std::forward<Args>(args)...);
      return;
    }

    sign_t sign = sign_t::positive;
    switch(source.peek())
    {
    case '-' :
      sign = sign_t::negative;
      source.skip();
      break;
    case '+' :
      source.skip();
      break;
    default :
      break;
    }

    next.submit(source, sign, std::forward<Args>(args)...);
  }
};
      
inline auto constexpr read_optional_sign = read_optional_sign_t{};

template<typename T, sign_t sign>
struct to_signed_t
{
  static_assert(std::is_unsigned_v<T>);

  template<typename Next, typename... Args>
  void operator()(Next next, async_source_t source, T value,
                  Args&&... args) const
  {
    std::make_signed_t<T> signed_value;

    if constexpr(sign == sign_t::positive)
    {
      signed_value = value;
    }
    else
    {
      if(value == 0)
      {
        signed_value = 0;
      }
      else
      {
        signed_value = value - 1;
        signed_value = -signed_value - 1;
      }
    }

    next.submit(source, signed_value, std::forward<Args>(args)...);
  }
};

template<typename T, sign_t sign>
auto constexpr to_signed = to_signed_t<T, sign>{};

template<typename T>
struct read_signed_digits_t
{
  static_assert(std::is_signed_v<T>);
  static_assert(std::is_integral_v<T>);

  template<typename Next, typename... Args>
  void operator()(Next next, async_source_t source, sign_t sign,
                  Args&&... args) const
  {
    using UT = std::make_unsigned_t<T>;
    static UT constexpr positive_limit = std::numeric_limits<T>::max();
    static UT constexpr negative_limit = positive_limit + 1;

    if(sign == sign_t::positive)
    {
      static auto constexpr chain = async_stitch(
        read_unsigned_digits<UT, positive_limit>,
        to_signed<UT, sign_t::positive>);
      chain(next, source, std::forward<Args>(args)...);
    }
    else
    {
      static auto constexpr chain = async_stitch(
        read_unsigned_digits<UT, negative_limit>,
        to_signed<UT, sign_t::negative>);
      chain(next, source, std::forward<Args>(args)...);
    }
  }
};
        
template<typename T>
auto constexpr read_signed_digits = read_signed_digits_t<T>{};

template<typename T>
auto constexpr read_signed = async_stitch(
  skip_whitespace,
  read_optional_sign,
  read_signed_digits<T>);

inline auto constexpr read_double_quote = read_fixed_char<'\"'>;

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
  template<typename Next, typename... Args>
  void operator()(Next next, async_source_t source,
                  unsigned int count, unsigned int total, std::string&& value,
                  Args&&... args) const
  {
    while(count != 0)
    {
      if(!source.readable())
      {
        source.call_when_readable(*this,
          next, source, count, total, std::move(value),
          std::forward<Args>(args)...);
        return;
      }

      int dval = hex_digit_value(source.peek());
      if(dval == -1)
      {
        next.fail(parse_error_t("hex digit expected"));
        return;
      }

      source.skip();
      total *= 16;
      total += dval;

      --count;
    }

    value += static_cast<char>(total);
    next.submit(source, std::move(value), std::forward<Args>(args)...);
  }
};

inline auto constexpr append_hex_digits = append_hex_digits_t{};

struct append_string_escape_t
{
  template<typename Next, typename... Args>
  void operator()(Next next, async_source_t source, std::string&& value,
                  Args&&... args) const
  {
    if(!source.readable())
    {
      source.call_when_readable(*this,
        next, source, std::move(value), std::forward<Args>(args)...);
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
    case '\"' :
      value += '\"'; break;
    case 'x' :
      {
        source.skip();
        append_hex_digits(next, source, 2, 0, std::move(value),
          std::forward<Args>(args)...);
        return;
      }
    default :
      {
        next.fail(parse_error_t("illegal escape sequence in string value"));
        return;
      }
    }
        
    source.skip();
    next.submit(source, std::move(value), std::forward<Args>(args)...);
  }
};    
          
inline auto constexpr append_string_escape = append_string_escape_t{};

inline bool is_literal_char(int c)
{
  switch(c)
  {
  case '\"' :
  case '\\' :
    return false;
  default :
    return c >= 0x20 && c <= 0xFF;
  }
}

struct append_string_chars_t
{
  static auto constexpr max_recursion = 100;

  template<typename Next, typename... Args>
  void operator()(Next next, async_source_t source,
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
        next, source, std::move(value), 0, std::forward<Args>(args)...);
      return;
    }

    if(c == '\\')
    {
      source.skip();
      static auto constexpr chain = async_stitch(
        append_string_escape, append_string_chars_t{});
      chain(next, source, std::move(value), recursion + 1,
        std::forward<Args>(args)...);
      return;
    }

    if(c == '\n' || c == eof)
    {
      next.fail(parse_error_t("missing terminating '\"'"));
      return;
    }

    if(c != '\"')
    {
      next.fail(parse_error_t(
        "illegal character " + std::to_string(c) + " in string value"));
      return;
    }

    source.skip();
    next.submit(source, std::move(value), std::forward<Args>(args)...);
  }
};

inline auto constexpr append_string_chars = append_string_chars_t{};

struct read_string_t
{
  template<typename Next, typename... Args>
  void operator()(Next next, async_source_t source, Args&&... args) const
  {
    static auto constexpr chain = async_stitch(
      skip_whitespace, read_double_quote, append_string_chars);
    chain(next, source, std::string{}, 0, std::forward<Args>(args)...);
  }
};
      
inline auto constexpr read_string = read_string_t{};

inline auto constexpr read_begin_sequence = read_fixed_char<'['>;

template<typename T>
struct append_element_t
{
  template<typename Next, typename TT, typename... Args>
  void operator()(Next next, async_source_t source,
                  TT&& element, std::vector<T>&& sequence,
                  Args&&... args) const
  {
    sequence.push_back(std::forward<TT>(element));
    next.submit(source, std::move(sequence), std::forward<Args>(args)...);
  }
};

template<typename T>
auto constexpr append_element = append_element_t<T>{};

template<typename T>
struct append_sequence_t
{
  static auto constexpr max_recursion = 100;

  template<typename Next, typename... Args>
  void operator()(Next next, async_source_t source,
                  std::vector<T>&& sequence, int recursion,
                  Args&&... args) const
  {
    if(!source.readable() || recursion == max_recursion)
    {
      source.call_when_readable(*this,
        next, source, std::move(sequence), 0, std::forward<Args>(args)...);
      return;
    }

    if(source.peek() != ']')
    {
      static auto constexpr chain = async_stitch(
        async_read<T>, append_element<T>,
        skip_whitespace, append_sequence_t<T>{});
      chain(next, source, std::move(sequence), recursion + 1,
        std::forward<Args>(args)...);
      return;
    }

    source.skip();
    next.submit(source, std::move(sequence), std::forward<Args>(args)...);
 }
};
      
template<typename T>
auto constexpr append_sequence = append_sequence_t<T>{};

template<typename T>
struct read_sequence_t
{
  template<typename Next, typename... Args>
  void operator()(Next next, async_source_t source, Args&&... args) const
  {
    static auto constexpr chain = async_stitch(skip_whitespace,
      read_begin_sequence, skip_whitespace, append_sequence<T>);
    chain(next, source, std::vector<T>{}, 0, std::forward<Args>(args)...);
  }
};

template<typename T>
auto constexpr read_sequence = read_sequence_t<T>{};

inline auto constexpr read_begin_struct = read_fixed_char<'{'>;

inline auto constexpr read_end_struct = read_fixed_char<'}'>;

template<typename... Fields>
struct read_fields_t;

template<>
struct read_fields_t<>
{
  template<typename Next, typename... Args>
  void operator()(Next next, async_source_t source, Args&&... args) const
  {
    next.submit(source, std::forward<Args>(args)...);
  }
};

template<typename FirstField, typename... OtherFields>
struct read_fields_t<FirstField, OtherFields...>
{
  template<typename Next, typename... Args>
  void operator()(Next next, async_source_t source, Args&&... args) const
  {
    static auto constexpr chain = async_stitch(
      async_read<FirstField>, read_fields_t<OtherFields...>{});
    chain(next, source, std::forward<Args>(args)...);
  }
};

template<typename... Fields>
auto constexpr read_fields = read_fields_t<Fields...>{};

template<typename T, int N>
struct build_impl_t;

template<typename T>
struct build_impl_t<T, 0>
{
  template<typename Next, typename Refs, typename... Args>
  void operator()(Next next, async_source_t source,
                  Refs refs, Args&&... args) const
  {
    std::optional<T> opt_value;

    try
    {
      static auto constexpr factory = construct<T>;
      opt_value.emplace(refs.apply(factory));
    }
    catch(std::exception const&)
    {
      next.fail(std::current_exception());
      return;
    }

    next.submit(source, std::move(*opt_value), std::forward<Args>(args)...);
  }
};

template<typename T, int N>
struct build_impl_t
{
  template<typename Next, typename Refs, typename Arg1, typename... Argn>
  void operator()(Next next, async_source_t source,
                  Refs refs, Arg1&& arg1, Argn&&... argn) const
  {
    static auto constexpr delegate = build_impl_t<T, N - 1>{};
    delegate(next, source,
      refs.with_first_arg(std::forward<Arg1>(arg1)),
      std::forward<Argn>(argn)...);
  }
};

template<typename T, int N>
struct build_t
{
  template<typename Next, typename... Args>
  void operator()(Next next, async_source_t source, Args&&... args) const
  {
    static auto constexpr impl = build_impl_t<T, N>{};
    impl(next, source, ref_args(), std::forward<Args>(args)...);
  }
};

template<typename T, int N>
auto constexpr build = build_t<T, N>{};

template<typename T, typename... Fields>
struct read_struct_t
{
  template<typename Next, typename... Args>
  void operator()(Next next, async_source_t source, Args&&... args) const
  {
    static auto constexpr chain = async_stitch(
      skip_whitespace,
      read_begin_struct,
      read_fields<Fields...>,
      build<T, sizeof...(Fields)>,
      skip_whitespace,
      read_end_struct
    );
    chain(next, source, std::forward<Args>(args)...);
  }
};
  
template<typename T, typename... Fields>
auto constexpr read_struct = read_struct_t<T, Fields...>{};

} // namespace cuti::detail

inline auto constexpr drop_source = detail::drop_source_t{};
inline auto constexpr read_eof = detail::read_eof_t{};

template<>
inline auto constexpr async_read<bool> =
  detail::read_bool;

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

template<typename T, typename... Fields>
auto constexpr async_read_struct =
  detail::read_struct<T, Fields...>;

} // namespace cuti

#endif
