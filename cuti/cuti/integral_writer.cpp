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

#include "integral_writer.hpp"

#include <utility>

namespace cuti
{

namespace detail
{

template<typename T>
unsigned_writer_t<T>::unsigned_writer_t(result_t<void>& result,
                                        bound_outbuf_t& buf)
: result_(result)
, prefix_writer_(*this, &unsigned_writer_t::on_failure, buf)
, digits_writer_(*this, &unsigned_writer_t::on_failure, buf)
, value_()
{ }

template<typename T>
void unsigned_writer_t<T>::start(T value)
{
  value_ = value;
  prefix_writer_.start(&unsigned_writer_t::on_prefix_written, " ");
}

template<typename T>
void unsigned_writer_t<T>::on_prefix_written()
{
  digits_writer_.start(&unsigned_writer_t::on_digits_written, value_);
}

template<typename T>
void unsigned_writer_t<T>::on_digits_written()
{
  result_.submit();
}

template<typename T>
void unsigned_writer_t<T>::on_failure(std::exception_ptr ex)
{
  result_.fail(std::move(ex));
}

template struct unsigned_writer_t<unsigned short>;
template struct unsigned_writer_t<unsigned int>;
template struct unsigned_writer_t<unsigned long>;
template struct unsigned_writer_t<unsigned long long>;

template<typename T>
signed_writer_t<T>::signed_writer_t(result_t<void>& result, bound_outbuf_t& buf)
: result_(result)
, prefix_writer_(*this, &signed_writer_t::on_failure, buf)
, digits_writer_(*this, &signed_writer_t::on_failure, buf)
, unsigned_value_()
{ }

template<typename T>
void signed_writer_t<T>::start(T value)
{
  if(value >= 0)
  {
    unsigned_value_ = value;

    prefix_writer_.start(&signed_writer_t::on_prefix_written, " ");
  }
  else
  {
    ++value;
    value = -value;
    unsigned_value_ = value;
    ++unsigned_value_;

    prefix_writer_.start(&signed_writer_t::on_prefix_written, " -");
  }  
}

template<typename T>
void signed_writer_t<T>::on_prefix_written()
{
  digits_writer_.start(&signed_writer_t::on_digits_written, unsigned_value_);
}

template<typename T>
void signed_writer_t<T>::on_digits_written()
{
  result_.submit();
}

template<typename T>
void signed_writer_t<T>::on_failure(std::exception_ptr ex)
{
  result_.fail(std::move(ex));
}

template struct signed_writer_t<short>;
template struct signed_writer_t<int>;
template struct signed_writer_t<long>;
template struct signed_writer_t<long long>;

} // detail

} // cuti
