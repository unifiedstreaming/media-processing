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

#include "reader_utils.hpp"

#include "charclass.hpp"
#include "parse_error.hpp"

#include <cassert>
#include <limits>
#include <string>

namespace cuti
{

namespace detail
{

token_finder_t::token_finder_t(result_t<int>& result, bound_inbuf_t& buf)
: result_(result)
, buf_(buf)
{ }

void token_finder_t::start()
{
  int c{};
  while(buf_.readable() && is_whitespace(c = buf_.peek()))
  {
    buf_.skip();
  }

  if(!buf_.readable())
  {
    buf_.call_when_readable([this] { this->start(); });
    return;
  }

  // TODO: check for inline exception in buf_ and fail if so 

  result_.submit(c);
}
  
template<typename T>
digits_reader_t<T>::digits_reader_t(result_t<T>& result, bound_inbuf_t& buf)
: result_(result)
, buf_(buf)
, max_()
, digit_seen_()
, value_()
{ }

template<typename T>
void digits_reader_t<T>::start(T max)
{
  max_ = max;
  digit_seen_ = false;
  value_ = 0;

  this->read_digits();
}

template<typename T>
void digits_reader_t<T>::read_digits()
{
  int dval{};
  while(buf_.readable() && (dval = digit_value(buf_.peek())) >= 0)
  {
    digit_seen_ = true;

    T udval = static_cast<T>(dval);
    if(value_ > max_ / 10 || udval > max_ - 10 * value_)
    {
      result_.fail(parse_error_t("integral type overflow"));
      return;
    }

    value_ *= 10;
    value_ += udval;

    buf_.skip();
  }

  if(!buf_.readable())
  {
    buf_.call_when_readable([this] { this->read_digits(); });
    return;
  }

  if(!digit_seen_)
  {
    result_.fail(parse_error_t("digit expected"));
    return;
  }
  
  result_.submit(value_);
}

template struct digits_reader_t<unsigned short>;
template struct digits_reader_t<unsigned int>;
template struct digits_reader_t<unsigned long>;
template struct digits_reader_t<unsigned long long>;

chunk_reader_t::chunk_reader_t(
  result_t<std::pair<char const*, char const*>>& result,
  bound_inbuf_t& buf)
: result_(result)
, buf_(buf)
, finder_(*this, &chunk_reader_t::on_exception, buf_)
, digits_reader_(*this, &chunk_reader_t::on_exception, buf_)
, data_(nullptr)
, next_(nullptr)
, last_(nullptr)
, edata_(nullptr)
{ }

void chunk_reader_t::start()
{
  finder_.start(&chunk_reader_t::read_lt);
}

chunk_reader_t::~chunk_reader_t()
{
  delete[] data_;
}

void chunk_reader_t::read_lt(int c)
{
  assert(buf_.readable());
  assert(buf_.peek() == c);

  if(c != '<')
  {
    result_.fail(parse_error_t("'<' expected"));
    return;
  }
  buf_.skip();

  digits_reader_.start(&chunk_reader_t::on_chunk_size,
                       std::numeric_limits<std::size_t>::max());
}

void chunk_reader_t::on_chunk_size(std::size_t chunk_size)
{
  if(chunk_size > max_chunk_size)
  {
    result_.fail(parse_error_t(
     "maximum chunk size (" + std::to_string(max_chunk_size) + ") exceeded"));
    return;
  }

  std::size_t data_size = edata_ - data_;
  if(data_size < chunk_size)
  {
    data_size =
      data_size <= max_chunk_size / 2 ? data_size * 2 : max_chunk_size;
    data_size =
      data_size <= max_chunk_size - 15 ? data_size + 15 : max_chunk_size;
    if(data_size < chunk_size)
    {
      data_size = chunk_size;
    }

    char* new_data = new char[data_size];
    delete[] data_;
    data_ = new_data;
    edata_ = data_ + data_size;
  }

  next_ = data_;
  last_ = data_ + chunk_size;

  this->read_gt();
}

void chunk_reader_t::read_gt()
{
  if(!buf_.readable())
  {
    buf_.call_when_readable([this] { this->read_gt(); });
    return;
  }

  if(buf_.peek() != '>')
  {
    result_.fail(parse_error_t("'>' expected"));
    return;
  }
  buf_.skip();

  this->read_data();
}

void chunk_reader_t::read_data()
{
  while(next_ != last_ && buf_.readable())
  {
    char *new_next = buf_.read(next_, last_);
    if(new_next == next_)
    {
      result_.fail(parse_error_t("unexpected eof in chunk data"));
      return;
    }

    next_ = new_next;
  }

  if(next_ != last_)
  {
    buf_.call_when_readable([this] { this->read_data(); });
    return;
  }

  result_.submit(std::make_pair(data_, last_));
}

void chunk_reader_t::on_exception(std::exception_ptr ex)
{
  result_.fail(std::move(ex));
}

} // detail

} // cuti
