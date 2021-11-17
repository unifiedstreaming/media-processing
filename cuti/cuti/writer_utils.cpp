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

#include "writer_utils.hpp"

#include "stack_marker.hpp"

#include <cassert>
#include <limits>
#include <utility>

namespace cuti
{

namespace detail
{

template<typename T>
digits_writer_t<T>::digits_writer_t(result_t<void>& result,
                                    bound_outbuf_t& buf)
: result_(result)
, buf_(buf)
, value_()
, divisor_()
{ }

template<typename T>
void digits_writer_t<T>::start(T value)
{
  static T constexpr max = std::numeric_limits<T>::max();

  value_ = value;
  divisor_ = 1;
  while(divisor_ <= max / 10 && divisor_ * 10 <= value_)
  {
    divisor_ *= 10;
  }

  this->write_digits();
}

template<typename T>
void digits_writer_t<T>::write_digits()
{
  while(divisor_ >= 1 && buf_.writable())
  {
    buf_.put(static_cast<char>((value_ / divisor_) + '0'));
    value_ %= divisor_;
    divisor_ /= 10;
  }

  if(divisor_ >= 1)
  {
    buf_.call_when_writable([this] { this->write_digits(); });
    return;
  }

  result_.submit();
}
      
template struct digits_writer_t<unsigned short>;
template struct digits_writer_t<unsigned int>;
template struct digits_writer_t<unsigned long>;
template struct digits_writer_t<unsigned long long>;

char const blob_prefix[] = " \"";
char const blob_suffix[] = "\"";

template<typename T>
blob_writer_t<T>::blob_writer_t(result_t<void>& result, bound_outbuf_t& buf)
: result_(result)
, buf_(buf)
, prefix_writer_(*this, &blob_writer_t::on_exception, buf_)
, suffix_writer_(*this, &blob_writer_t::on_exception, buf_)
, value_()
, first_()
, last_()
{ }

template<typename T>
void blob_writer_t<T>::start(T value)
{
  value_ = std::move(value);
  first_ = value_.begin();
  last_ = value_.end();
    
  prefix_writer_.start(&blob_writer_t::write_contents);
}

template<typename T>
void blob_writer_t<T>::write_contents()
{
  while(first_ != last_ && buf_.writable())
  {
    switch(*first_)
    {
    case '\n' :
    case '\"' :
    case '\\' :
      buf_.put('\\');
      this->write_escaped();
      return;
    default :
      buf_.put(static_cast<char>(*first_));
      break;
    }
    ++first_;
  }

  if(first_ != last_)
  {
    buf_.call_when_writable([this] { this->write_contents(); });
    return;
  }

  suffix_writer_.start(&blob_writer_t::on_suffix_written);
}

template<typename T>
void blob_writer_t<T>::write_escaped()
{
  if(!buf_.writable())
  {
    buf_.call_when_writable([this] { this->write_escaped(); });
    return;
  }
    
  assert(first_ != last_);
  switch(*first_)
  {
  case '\n' :
    buf_.put('n');
    break;
  case '\"' :
    buf_.put('\"');
    break;
  case '\\' :
    buf_.put('\\');
    break;
  default :
    assert(false);
    break;
  }
  ++first_;

  stack_marker_t marker;
  if(marker.in_range(buf_.base_marker()))
  {
    this->write_contents();
    return;
  }

  buf_.call_when_writable([this] { this->write_contents(); });
}

template<typename T>
void blob_writer_t<T>::on_suffix_written()
{
  value_.clear();
  result_.submit();
}

template<typename T>
void blob_writer_t<T>::on_exception(std::exception_ptr ex)
{
  result_.fail(std::move(ex));
}

template struct blob_writer_t<std::string>;
template struct blob_writer_t<std::vector<char>>;
template struct blob_writer_t<std::vector<signed char>>;
template struct blob_writer_t<std::vector<unsigned char>>;

} // detail

} // cuti
