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

#ifndef CUTI_BOOLEAN_WRITER_HPP_
#define CUTI_BOOLEAN_WRITER_HPP_

#include "bound_outbuf.hpp"
#include "flag.hpp"
#include "linkage.h"
#include "result.hpp"
#include "subroutine.hpp"
#include "writer_traits.hpp"
#include "writer_utils.hpp"

#include <exception>

namespace cuti
{

namespace detail
{

template<typename T>
struct CUTI_ABI boolean_writer_t
{
  using result_value_t = void;

  boolean_writer_t(result_t<void>& result, bound_outbuf_t& buf);

  boolean_writer_t(boolean_writer_t const&) = delete;
  boolean_writer_t& operator=(boolean_writer_t const&) = delete;
  
  void start(T value);

private :
  void on_literal_written();
  void on_exception(std::exception_ptr ex);

private :
  result_t<void>& result_;
  subroutine_t<boolean_writer_t, literal_writer_t> literal_writer_;
};

extern template struct boolean_writer_t<bool>;
extern template struct boolean_writer_t<flag_t>;

} // detail

template<>
struct writer_traits_t<bool>
{
  using type = detail::boolean_writer_t<bool>;
};

template<>
struct writer_traits_t<flag_t>
{
  using type = detail::boolean_writer_t<flag_t>;
};

} // cuti

#endif
