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

template<typename T>
chunk_reader_t<T>::chunk_reader_t(result_t<std::size_t>& result,
                                  bound_inbuf_t& buf)
: result_(result)
, buf_(buf)
, finder_(*this, &chunk_reader_t::on_exception, buf_)
, digits_reader_(*this, &chunk_reader_t::on_exception, buf_)
, target_()
, first_()
, next_()
, last_()
{ }

template<typename T>
void chunk_reader_t<T>::start(std::vector<T>& target)
{
  target_ = &target;
  finder_.start(&chunk_reader_t::read_lt);
}

template<typename T>
void chunk_reader_t<T>::read_lt(int c)
{
  assert(buf_.readable());
  assert(buf_.peek() == c);

  if(c != '<')
  {
    result_.fail(parse_error_t("'<' expected"));
    return;
  }
  buf_.skip();

  digits_reader_.start(&chunk_reader_t::on_chunksize,
                       std::numeric_limits<std::size_t>::max());
}

template<typename T>
void chunk_reader_t<T>::on_chunksize(std::size_t chunksize)
{
  if(chunksize > max_chunksize)
  {
    result_.fail(parse_error_t("maximum chunk size (" +
      std::to_string(max_chunksize) + ") exceeded"));
    return;
  }

  std::size_t initial_size = target_->size();
  if(chunksize > target_->max_size() - initial_size)
  {
    result_.fail(parse_error_t("maximum vector size (" +
      std::to_string(target_->max_size()) + ") exceeded"));
    return;
  }

  target_->resize(initial_size + chunksize);

  first_ = reinterpret_cast<char*>(target_->data()) + initial_size;
  next_ = first_;
  last_ = next_ + chunksize;

  this->read_gt();
}

template<typename T>
void chunk_reader_t<T>::read_gt()
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

template<typename T>
void chunk_reader_t<T>::read_data()
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

  result_.submit(last_ - first_);
}

template<typename T>
void chunk_reader_t<T>::on_exception(std::exception_ptr ex)
{
  result_.fail(std::move(ex));
}

template struct chunk_reader_t<char>;
template struct chunk_reader_t<signed char>;
template struct chunk_reader_t<unsigned char>;

} // detail

} // cuti
