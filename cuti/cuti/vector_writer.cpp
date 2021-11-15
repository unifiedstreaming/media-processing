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

#include "vector_writer.hpp"

#include "stack_marker.hpp"

#include <cassert>
#include <utility>

namespace cuti
{

namespace detail
{

template<typename T>
bulk_writer_t<T>::bulk_writer_t(result_t<void>& result,
                                bound_outbuf_t& buf,
                                std::size_t chunksize)
: result_(result)
, buf_(buf)
, chunksize_((assert(chunksize != 0), chunksize))
, chunk_writer_(*this, &bulk_writer_t::on_exception, buf_)
, value_()
, first_()
, last_()
{ }

template<typename T>
void bulk_writer_t<T>::start(std::vector<T> value)
{
  value_ = std::move(value);
  first_ = value_.data();
  last_ = first_ + value_.size();

  this->write_chunks();
}

template<typename T>
void bulk_writer_t<T>::write_chunks()
{
  if(first_ != last_)
  {
    std::size_t chunksize = last_ - first_;
    if(chunksize > chunksize_)
    {
      chunksize = chunksize_;
    }

    T const* first = first_;
    T const* last = first + chunksize;
    first_ = last;

    chunk_writer_.start(
      &bulk_writer_t::on_intermediate_chunk_written, first, last);
    return;
  }
  
  chunk_writer_.start(
    &bulk_writer_t::on_final_chunk_written, first_, last_);
}

template<typename T>
void bulk_writer_t<T>::on_intermediate_chunk_written()
{
  stack_marker_t marker;
  if(marker.in_range(buf_.base_marker()))
  {
    this->write_chunks();
    return;
  }

  buf_.call_when_writable([this] { this->write_chunks(); });
}

template<typename T>
void bulk_writer_t<T>::on_final_chunk_written()
{
  result_.submit();
}

template<typename T>
void bulk_writer_t<T>::on_exception(std::exception_ptr ex)
{
  result_.fail(std::move(ex));
}

template struct bulk_writer_t<char>;
template struct bulk_writer_t<signed char>;
template struct bulk_writer_t<unsigned char>;

} // detail

} // cuti
