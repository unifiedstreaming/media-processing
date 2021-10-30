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

#ifndef CUTI_WHITESPACE_SKIPPER_HPP_
#define CUTI_WHITESPACE_SKIPPER_HPP_

#include "bound_inbuf.hpp"
#include "charclass.hpp"
#include "linkage.h"
#include "result.hpp"

namespace cuti
{

struct CUTI_ABI whitespace_skipper_t
{
  using value_t = no_value_t;

  whitespace_skipper_t(result_t<no_value_t>& result, bound_inbuf_t& buf)
  : result_(result)
  , buf_(buf)
  { }

  whitespace_skipper_t(whitespace_skipper_t const&) = delete;
  whitespace_skipper_t& operator=(whitespace_skipper_t const&) = delete;
  
  void start()
  {
    while(buf_.readable() && is_whitespace(buf_.peek()))
    {
      buf_.skip();
    }

    if(!buf_.readable())
    {
      buf_.call_when_readable([this] { this->start(); });
      return;
    }

    result_.submit();
  }

private :
  result_t<no_value_t>& result_;
  bound_inbuf_t& buf_;
};

} // cuti

#endif
