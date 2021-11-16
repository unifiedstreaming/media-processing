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

#include "string_writer.hpp"

#include "charclass.hpp"
#include "stack_marker.hpp"

#include <cassert>
#include <utility>

namespace cuti
{

namespace detail
{

string_writer_t::string_writer_t(result_t<void>& result, bound_outbuf_t& buf)
: result_(result)
, buf_(buf)
, prefix_writer_(*this, &string_writer_t::on_exception, buf_)
, value_()
, first_()
, last_()
{ }

void string_writer_t::start(std::string value)
{
  value_ = std::move(value);
  first_ = value_.data();
  last_ = first_ + value_.size();
    
  prefix_writer_.start(&string_writer_t::write_contents, " \"");
}

void string_writer_t::write_contents()
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
      buf_.put(*first_);
      break;
    }
    ++first_;
  }

  if(first_ != last_)
  {
    buf_.call_when_writable([this] { this->write_contents(); });
    return;
  }

  this->write_closing_dq();
}

void string_writer_t::write_escaped()
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

void string_writer_t::write_closing_dq()
{
  if(!buf_.writable())
  {
    buf_.call_when_writable([this] { this->write_closing_dq(); });
    return;
  }
  buf_.put('\"');

  value_.clear();
  result_.submit();
}

void string_writer_t::on_exception(std::exception_ptr ex)
{
  result_.fail(std::move(ex));
}

} // detail

} // cuti
