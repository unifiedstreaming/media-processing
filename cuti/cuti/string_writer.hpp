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

#include <string>

namespace cuti
{

struct CUTI_ABI string_writer_t
{
  using value_t = void;

  string_writer_t(result_t<void>& result, bound_outbuf_t& buf);

  string_writer_t(string_writer_t const&) = delete;
  string_writer_t& operator=(string_writer_t const&) = delete;
  
  void start(std::string value);

private :
  struct CUTI_ABI char_escape_writer_t
  {
    using value_t = void;

    char_escape_writer_t(result_t<void>& result_, bound_outbuf_t& buf);

    char_escape_writer_t(char_escape_writer_t const&) = delete;
    char_escape_writer_t& operator=(char_escape_writer_t const&) = delete;

    void start(char value);

  private :
    void write_backslash();
    void write_value();

  private :
    result_t<void>& result_;
    bound_outbuf_t& buf_;

    char value_;
  };
    
  struct CUTI_ABI hex_escape_writer_t
  {
    using value_t = void;

    hex_escape_writer_t(result_t<void>& result_, bound_outbuf_t& buf);

    hex_escape_writer_t(hex_escape_writer_t const&) = delete;
    hex_escape_writer_t& operator=(hex_escape_writer_t const&) = delete;

    void start(int value);

  private :
    void write_backslash();
    void write_x();
    void write_hex_digits();

  private :
    result_t<void>& result_;
    bound_outbuf_t& buf_;

    int value_;
    int shift_;
  };
    
private :
  void write_space();
  void write_opening_dq();
  void write_contents();
  void write_closing_dq();
  void on_exception(std::exception_ptr ex);

private :
  result_t<void>& result_;
  bound_outbuf_t& buf_;
  subroutine_t<string_writer_t, char_escape_writer_t> char_escape_writer_;
  subroutine_t<string_writer_t, hex_escape_writer_t> hex_escape_writer_;
  
  std::string value_;
  char const* rp_;
  char const* ep_;
  int recursion_;
};

} // cuti

#endif
