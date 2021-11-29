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
#include "linkage.h"
#include "reader_traits.hpp"
#include "reader_utils.hpp"
#include "result.hpp"
#include "sequence_reader.hpp"
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
  , begin_reader_(*this, result_, buf)
  , end_checker_(*this, result_, buf)
  , element_reader_(*this, result_, buf)
  , value_()
  { }

  void start()
  {
    value_.clear();
    begin_reader_.start(&vector_reader_t::read_elements);
  }

private :
  void read_elements()
  {
    end_checker_.start(&vector_reader_t::on_end_checker);
  }

  void on_end_checker(bool at_end)
  {
    if(at_end)
    {
      result_.submit(std::move(value_));
      return;
    }
      
    element_reader_.start(&vector_reader_t::on_element);
  }
    
  void on_element(T element)
  {
    value_.push_back(std::move(element));
    this->read_elements();
  }

private :
  result_t<std::vector<T>>& result_;
  subroutine_t<vector_reader_t, begin_sequence_reader_t> begin_reader_;
  subroutine_t<vector_reader_t, end_sequence_checker_t> end_checker_;
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
