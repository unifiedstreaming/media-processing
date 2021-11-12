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

#ifndef CUTI_STRING_WRITER_HPP_
#define CUTI_STRING_WRITER_HPP_

#include "bound_outbuf.hpp"
#include "linkage.h"
#include "result.hpp"
#include "subroutine.hpp"
#include "writer_traits.hpp"
#include "writer_utils.hpp"

#include <exception>
#include <string>

namespace cuti
{

namespace detail
{

struct CUTI_ABI hex_digits_writer_t
{
  using result_value_t = void;

  hex_digits_writer_t(result_t<void>& result_, bound_outbuf_t& buf);

  hex_digits_writer_t(hex_digits_writer_t const&) = delete;
  hex_digits_writer_t& operator=(hex_digits_writer_t const&) = delete;

  void start(int value);

private :
  void write_digits();

private :
  result_t<void>& result_;
  bound_outbuf_t& buf_;

  int value_;
  int shift_;
};
    
struct CUTI_ABI string_writer_t
{
  using result_value_t = void;

  string_writer_t(result_t<void>& result, bound_outbuf_t& buf);

  string_writer_t(string_writer_t const&) = delete;
  string_writer_t& operator=(string_writer_t const&) = delete;
  
  void start(std::string value);

private :
  void write_contents();
  void write_escaped();
  void write_closing_dq();
  void on_escaped_written();
  void on_exception(std::exception_ptr ex);

private :
  result_t<void>& result_;
  bound_outbuf_t& buf_;
  subroutine_t<string_writer_t, literal_writer_t> prefix_writer_;
  subroutine_t<string_writer_t, hex_digits_writer_t> hex_digits_writer_;
  
  std::string value_;
  char const* first_;
  char const* last_;
};

} // detail

template<>
struct writer_traits_t<std::string>
{
  using type = detail::string_writer_t;
};

} // cuti

#endif
