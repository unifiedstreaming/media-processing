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

#include "vector_reader.hpp"

#include "stack_marker.hpp"

#include <utility>

namespace cuti
{

namespace detail
{

template<typename T>
bulk_reader_t<T>::bulk_reader_t(
  result_t<std::vector<T>>& result, bound_inbuf_t& buf)
: result_(result)
, buf_(buf)
, chunk_reader_(*this, &bulk_reader_t::on_exception, buf_)
, value_()
{ }

template<typename T>
void bulk_reader_t<T>::start()
{
  value_.clear();
  this->read_chunks();
}

template<typename T>
void bulk_reader_t<T>::read_chunks()
{
  chunk_reader_.start(&bulk_reader_t::on_chunk_read, value_);
}

template<typename T>
void bulk_reader_t<T>::on_chunk_read(std::size_t size)
{
  if(size != 0)
  {
    stack_marker_t marker;
    if(marker.in_range(buf_.base_marker()))
    {
      this->read_chunks();
      return;
    }

    buf_.call_when_readable([this] { this->read_chunks(); });
    return;
  }

  result_.submit(std::move(value_));
}

template<typename T>
void bulk_reader_t<T>::on_exception(std::exception_ptr ex)
{
  result_.fail(std::move(ex));
}

template struct bulk_reader_t<char>;
template struct bulk_reader_t<signed char>;
template struct bulk_reader_t<unsigned char>;

} // detail

} // cuti
