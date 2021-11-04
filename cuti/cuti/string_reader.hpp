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

#ifndef CUTI_STRING_READER_HPP_
#define CUTI_STRING_READER_HPP_

#include "bound_inbuf.hpp"
#include "linkage.h"
#include "result.hpp"
#include "subroutine.hpp"

#include <string>

namespace cuti
{

struct CUTI_ABI string_reader_t
{
  using value_t = std::string;

  string_reader_t(result_t<std::string>& result, bound_inbuf_t& buf);

  string_reader_t(string_reader_t const&) = delete;
  string_reader_t& operator=(string_reader_t const&) = delete;

  void start();

private :
  struct CUTI_ABI hex_digits_reader_t
  {
    using value_t = char;

    hex_digits_reader_t(result_t<char>& result, bound_inbuf_t& buf);

    hex_digits_reader_t(hex_digits_reader_t const&) = delete;
    hex_digits_reader_t& operator=(hex_digits_reader_t const&) = delete;

    void start();

  private :
    void read_digits();

  private :
    result_t<char>& result_;
    bound_inbuf_t& buf_;
    int shift_;
    char value_;
  };
    
  void read_opening_dq();
  void read_contents();
  void read_escaped();
  void on_char_value(char c);
  void on_exception(std::exception_ptr ex);
  
private :
  result_t<std::string>& result_;
  bound_inbuf_t& buf_;
  subroutine_t<string_reader_t, hex_digits_reader_t> hex_digits_reader_;

  std::string value_;
  int recursion_;
};

} // cuti

#endif
