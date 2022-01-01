/*
 * Copyright (C) 2021-2022 CodeShop B.V.
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

#include "async_writers.hpp"

#include "remote_error.hpp"
#include "stack_marker.hpp"

#include <cassert>
#include <limits>
#include <optional>
#include <utility>

namespace cuti
{

namespace detail
{

char const space_suffix[] = " ";

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

char const true_literal[] = "| ";
char const false_literal[] = "& ";

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

char const blob_suffix[] = "\" ";

template<typename T>
unsigned_writer_t<T>::unsigned_writer_t(result_t<void>& result,
                                        bound_outbuf_t& buf)
: result_(result)
, digits_writer_(*this, result_, buf)
, space_writer_(*this, result_, buf)
{ }

template<typename T>
void unsigned_writer_t<T>::start(T value)
{
  digits_writer_.start(&unsigned_writer_t::on_digits_written, value);
}

template<typename T>
void unsigned_writer_t<T>::on_digits_written()
{
  space_writer_.start(&unsigned_writer_t::on_space_written);
}

template<typename T>
void unsigned_writer_t<T>::on_space_written()
{
  result_.submit();
}

template struct unsigned_writer_t<unsigned short>;
template struct unsigned_writer_t<unsigned int>;
template struct unsigned_writer_t<unsigned long>;
template struct unsigned_writer_t<unsigned long long>;

template<typename T>
signed_writer_t<T>::signed_writer_t(result_t<void>& result, bound_outbuf_t& buf)
: result_(result)
, buf_(buf)
, digits_writer_(*this, result_, buf_)
, space_writer_(*this, result_, buf_)
, unsigned_value_()
{ }

template<typename T>
void signed_writer_t<T>::start(T value)
{
  if(value >= 0)
  {
    unsigned_value_ = value;
    digits_writer_.start(
      &signed_writer_t::on_digits_written, unsigned_value_);
  }
  else
  {
    ++value;
    value = -value;
    unsigned_value_ = value;
    ++unsigned_value_;
    this->write_minus();
  }  
}

template<typename T>
void signed_writer_t<T>::write_minus()
{
  if(!buf_.writable())
  {
    buf_.call_when_writable([this] { this->write_minus(); });
    return;
  }
  buf_.put('-');

  digits_writer_.start(
    &signed_writer_t::on_digits_written, unsigned_value_);
}

template<typename T>
void signed_writer_t<T>::on_digits_written()
{
  space_writer_.start(&signed_writer_t::on_space_written);
}

template<typename T>
void signed_writer_t<T>::on_space_written()
{
  result_.submit();
}

template struct signed_writer_t<short>;
template struct signed_writer_t<int>;
template struct signed_writer_t<long>;
template struct signed_writer_t<long long>;

template<typename T>
blob_writer_t<T>::blob_writer_t(result_t<void>& result, bound_outbuf_t& buf)
: result_(result)
, buf_(buf)
, suffix_writer_(*this, result_, buf_)
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
    
  this->write_opening_dq();
}

template<typename T>
void blob_writer_t<T>::write_opening_dq()
{
  if(!buf_.writable())
  {
    buf_.call_when_writable([this] { this->write_opening_dq(); });
    return;
  }
  buf_.put('\"');

  this->write_contents();
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

template struct blob_writer_t<std::string>;
template struct blob_writer_t<std::vector<char>>;
template struct blob_writer_t<std::vector<signed char>>;
template struct blob_writer_t<std::vector<unsigned char>>;

identifier_writer_t::identifier_writer_t(result_t<void>& result,
                                         bound_outbuf_t& buf)
: result_(result)
, buf_(buf)
, space_writer_(*this, result_, buf)
, value_()
, begin_()
, end_()
{ }

void identifier_writer_t::start(identifier_t value)
{
  assert(value.is_valid());
  value_ = std::move(value);

  std::string const& rep = value_.as_string();
  begin_ = rep.begin();
  end_ = rep.end();

  this->write_contents();
}

void identifier_writer_t::write_contents()
{
  while(begin_ != end_ && buf_.writable())
  {
    buf_.put(*begin_);
    ++begin_;
  }

  if(begin_ != end_)
  {
    buf_.call_when_writable([this] { this->write_contents(); });
    return;
  }

  space_writer_.start(&identifier_writer_t::on_space_written);
}

void identifier_writer_t::on_space_written()
{
  value_ = identifier_t{};
  result_.submit();
}
  
char const sequence_prefix[] = "[ ";
char const sequence_suffix[] = "] ";

char const structure_prefix[] = "{ ";
char const structure_suffix[] = "} ";

namespace // anoymous
{

char const exception_marker[] = "! ";

} // anonymous

struct exception_writer_t::impl_t
{
  impl_t(result_t<void>& result, bound_outbuf_t& buf)
  : result_(result)
  , marker_writer_(*this, result_, buf)
  , error_writer_(*this, result_, buf)
  , error_(std::nullopt)
  { }

  impl_t(impl_t const&) = delete;
  impl_t& operator=(impl_t const&) = delete;

  void start(remote_error_t error)
  {
    error_.emplace(std::move(error));
    marker_writer_.start(&impl_t::on_marker_written);
  }

private :
  void on_marker_written()
  {
    assert(error_ != std::nullopt);
    error_writer_.start(&impl_t::on_error_written, std::move(*error_));
  }

  void on_error_written()
  {
    error_.reset();
    result_.submit();
  }

private :
  result_t<void>& result_;
  subroutine_t<impl_t, token_suffix_writer_t<exception_marker>>
    marker_writer_;
  subroutine_t<impl_t, writer_t<remote_error_t>>
    error_writer_;

  std::optional<remote_error_t> error_;
};

exception_writer_t::exception_writer_t(result_t<void>& result,
                                       bound_outbuf_t& buf)
: impl_(std::make_unique<impl_t>(result, buf))
{ }

void exception_writer_t::start(remote_error_t error)
{
  impl_->start(std::move(error));
}

exception_writer_t::~exception_writer_t()
{ }
    
char const newline[] = "\n";

eom_writer_t::eom_writer_t(result_t<void>& result, bound_outbuf_t& buf)
: result_(result)
, newline_writer_(*this, result_, buf)
, flusher_(*this, result_, buf)
{ }

void eom_writer_t::start()
{
  newline_writer_.start(&eom_writer_t::on_newline_written);
}

void eom_writer_t::on_newline_written()
{
  flusher_.start(&eom_writer_t::on_flushed);
}

void eom_writer_t::on_flushed()
{
  result_.submit();
}

} // detail

} // cuti
