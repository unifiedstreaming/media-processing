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

#include "boolean_writers.hpp"

namespace cuti
{

namespace detail
{

template<typename T>
boolean_writer_t<T>::boolean_writer_t(
  result_t<void>& result, bound_outbuf_t& buf)
: result_(result)
, buf_(buf)
, value_()
{ }

template<typename T>
void boolean_writer_t<T>::start(T value)
{
  value_ = value;
  this->write_space();
}

template<typename T>
void boolean_writer_t<T>::write_space()
{
  if(!buf_.writable())
  {
    buf_.call_when_writable([this] { this->write_space(); });
    return;
  }
  buf_.put(' ');

  this->write_value();
}

template<typename T>
void boolean_writer_t<T>::write_value()
{
  if(!buf_.writable())
  {
    buf_.call_when_writable([this] { this->write_value(); });
    return;
  }
  buf_.put(value_ ? '*' : '!');

  result_.submit();
}

template struct boolean_writer_t<bool>;
template struct boolean_writer_t<flag_t>;

} // detail

} // cuti
