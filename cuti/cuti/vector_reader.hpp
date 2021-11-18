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

#ifndef CUTI_VECTOR_READER_HPP_
#define CUTI_VECTOR_READER_HPP_

#include "bound_inbuf.hpp"
#include "charclass.hpp"
#include "eof.hpp"
#include "linkage.h"
#include "parse_error.hpp"
#include "reader_traits.hpp"
#include "reader_utils.hpp"
#include "result.hpp"
#include "stack_marker.hpp"
#include "subroutine.hpp"

#include <cassert>
#include <cstddef>
#include <utility>
#include <vector>

namespace cuti
{

namespace detail
{

template<typename T>
struct vector_reader_t
{
  using result_value_t = std::vector<T>;

  vector_reader_t(result_t<std::vector<T>>& result, bound_inbuf_t& buf)
  : result_(result)
  , buf_(buf)
  , finder_(*this, result_, buf)
  , element_reader_(*this, result_, buf)
  , value_()
  { }

  void start()
  {
    value_.clear();
    finder_.start(&vector_reader_t::on_begin_token);
  }

private :
  void on_begin_token(int c)
  {
    assert(buf_.readable());
    assert(buf_.peek() == c);

    if(c != '[')
    {
      result_.fail(parse_error_t("\'[\' expected"));
      return;
    }
    buf_.skip();

    this->read_elements();
  }

  void read_elements()
  {
    int c{};
    while(buf_.readable() && is_whitespace(c = buf_.peek()))
    {
      /*
       * Direct whitespace skipping (not using a token finder) is OK
       * here because the element reader should use a token finder to
       * check for inline exceptions in buf_.
       */
      buf_.skip();
    }

    if(!buf_.readable())
    {
      buf_.call_when_readable([this] { this->read_elements(); });
      return;
    }

    switch(c)
    {
    case ']' :
      break;
    case eof :
    case '\n' :
      result_.fail(parse_error_t("missing \']\' at end of sequence"));
      return;
    default :
      element_reader_.start(&vector_reader_t::on_element_read);
      return;
    }
      
    buf_.skip();
    result_.submit(std::move(value_));
  }
    
  void on_element_read(T element)
  {
    value_.push_back(std::move(element));

    stack_marker_t marker;
    if(marker.in_range(buf_.base_marker()))
    {
      this->read_elements();
      return;
    }

    buf_.call_when_readable([this] { this->read_elements(); });
  }

private :
  result_t<std::vector<T>>& result_;
  bound_inbuf_t& buf_;
  subroutine_t<vector_reader_t, token_finder_t> finder_;
  subroutine_t<vector_reader_t, reader_t<T>> element_reader_;

  std::vector<T> value_;
};

} // detail

template<typename T>
struct reader_traits_t<std::vector<T>>
{
  using type = detail::vector_reader_t<T>;
};

template<>
struct reader_traits_t<std::vector<char>>
{
  using type = detail::blob_reader_t<std::vector<char>>;
};

template<>
struct reader_traits_t<std::vector<signed char>>
{
  using type = detail::blob_reader_t<std::vector<signed char>>;
};

template<>
struct reader_traits_t<std::vector<unsigned char>>
{
  using type = detail::blob_reader_t<std::vector<unsigned char>>;
};

} // cuti

#endif
