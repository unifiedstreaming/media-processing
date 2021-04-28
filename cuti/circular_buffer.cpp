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

#include "circular_buffer.hpp"

#include <algorithm>
#include <limits>
#include <utility>

namespace cuti
{

circular_buffer_t::circular_buffer_t() noexcept
: empty_(true)
, buf_(nullptr)
, data_(nullptr)
, slack_(nullptr)
, end_(nullptr)
{ }

circular_buffer_t::circular_buffer_t(std::size_t capacity)
: empty_(true)
, buf_(capacity == 0 ? nullptr : new char[capacity])
, data_(buf_)
, slack_(buf_)
, end_(buf_ + capacity)
{ }

circular_buffer_t::circular_buffer_t(circular_buffer_t const& rhs)
: empty_(true)
, buf_(rhs.buf_ == nullptr ? nullptr : new char[rhs.end_ - rhs.buf_])
, data_(buf_)
, slack_(buf_)
, end_(buf_ + (rhs.end_ - rhs.buf_))
{
  if(!rhs.empty_)
  {
    empty_ = false;
    if(rhs.data_ < rhs.slack_)
    {
      slack_ = std::copy(rhs.data_, rhs.slack_, slack_);
    }
    else
    {
      slack_ = std::copy(rhs.data_, rhs.end_, slack_);
      slack_ = std::copy(rhs.buf_, rhs.slack_, slack_);
      if(slack_ == end_)
      {
        slack_ = buf_;
      }
    }
  }
}

circular_buffer_t::circular_buffer_t(circular_buffer_t&& rhs) noexcept
: empty_(rhs.empty_)
, buf_(rhs.buf_)
, data_(rhs.data_)
, slack_(rhs.slack_)
, end_(rhs.end_)
{
  rhs.empty_ = true;
  rhs.buf_ = nullptr;
  rhs.data_ = nullptr;
  rhs.slack_ = nullptr;
  rhs.end_ = nullptr;
}

circular_buffer_t& circular_buffer_t::operator=(circular_buffer_t const& rhs)
{
  circular_buffer_t tmp(rhs);
  this->swap(tmp);
  return *this;
}

circular_buffer_t& circular_buffer_t::operator=(circular_buffer_t&& rhs)
  noexcept
{
  circular_buffer_t tmp(std::move(rhs));
  this->swap(tmp);
  return *this;
}

void circular_buffer_t::swap(circular_buffer_t& that) noexcept
{
  using std::swap;

  swap(this->empty_, that.empty_);
  swap(this->buf_, that.buf_);
  swap(this->data_, that.data_);
  swap(this->slack_, that.slack_);
  swap(this->end_, that.end_);
}
  
circular_buffer_t::~circular_buffer_t()
{
  delete[] buf_;
}

void swap(circular_buffer_t& b1, circular_buffer_t& b2) noexcept
{
  b1.swap(b2);
}

} // cuti
