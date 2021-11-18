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

#include "boolean_writer.hpp"

#include <utility>

namespace cuti
{

namespace detail
{

char const true_literal[] = " *";
char const false_literal[] = " !";

template<typename T>
boolean_writer_t<T>::boolean_writer_t(
  result_t<void>& result, bound_outbuf_t& buf)
: result_(result)
, true_writer_(*this, result_, buf)
, false_writer_(*this, result_, buf)
{ }

template<typename T>
void boolean_writer_t<T>::start(T value)
{
  if(value)
  {
    true_writer_.start(&boolean_writer_t::on_done);
  }
  else
  {
    false_writer_.start(&boolean_writer_t::on_done);
  }
}

template<typename T>
void boolean_writer_t<T>::on_done()
{
  result_.submit();
}

template struct boolean_writer_t<bool>;
template struct boolean_writer_t<flag_t>;

} // detail

} // cuti
