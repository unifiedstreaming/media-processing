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

#ifndef CUTI_EOF_READER_HPP_
#define CUTI_EOF_READER_HPP_

#include "bound_inbuf.hpp"
#include "linkage.h"
#include "result.hpp"

namespace cuti
{

struct CUTI_ABI eof_reader_t
{
  eof_reader_t(result_t<>& result, bound_inbuf_t& inbuf);

  eof_reader_t(eof_reader_t const&) = delete;
  eof_reader_t& operator=(eof_reader_t const&) = delete;
  
  void start();

private :
  result_t<>& result_;
  bound_inbuf_t& buf_;
};

} // of namespace cuti

#endif
