/*
 * Copyright (C) 2021-2025 CodeShop B.V.
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

#ifndef CUTI_FLUSHER_HPP_
#define CUTI_FLUSHER_HPP_

#include "bound_outbuf.hpp"
#include "linkage.h"
#include "result.hpp"
#include "stack_marker.hpp"

namespace cuti
{

struct CUTI_ABI flusher_t
{
  using result_value_t = void;

  flusher_t(result_t<void>& result, bound_outbuf_t& buf);

  flusher_t(flusher_t const&) = delete;
  flusher_t& operator=(flusher_t const&) = delete;
  
  void start(stack_marker_t& base_marker);

private :
  void check_flushed(stack_marker_t& base_marker);

private :
  result_t<void>& result_;
  bound_outbuf_t& buf_;
};

} // of namespace cuti

#endif
