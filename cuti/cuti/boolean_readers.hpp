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

#ifndef CUTI_BOOLEAN_READERS_HPP_
#define CUTI_BOOLEAN_READERS_HPP_

#include "bound_inbuf.hpp"
#include "flag.hpp"
#include "linkage.h"
#include "reader_traits.hpp"
#include "reader_utils.hpp"
#include "result.hpp"
#include "subroutine.hpp"

#include <exception>

namespace cuti
{

namespace detail
{

template<typename T>
struct CUTI_ABI boolean_reader_t
{
  using value_t = T;

  boolean_reader_t(result_t<T>& result, bound_inbuf_t& buf);

  boolean_reader_t(boolean_reader_t const&) = delete;
  boolean_reader_t& operator=(boolean_reader_t const&) = delete;
  
  void start();

private :
  void on_begin_token(int c);
  void on_failure(std::exception_ptr ex);

private :
  result_t<T>& result_;
  bound_inbuf_t& buf_;
  subroutine_t<boolean_reader_t, token_finder_t> finder_;
};

extern template struct boolean_reader_t<bool>;
extern template struct boolean_reader_t<flag_t>;

} // detail

template<>
struct reader_traits_t<bool>
{
  using type = detail::boolean_reader_t<bool>;
};

template<>
struct reader_traits_t<flag_t>
{
  using type = detail::boolean_reader_t<flag_t>;
};

} // cuti

#endif
