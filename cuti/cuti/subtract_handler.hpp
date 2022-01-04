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

#ifndef CUTI_SUBTRACT_HANDLER_HPP_
#define CUTI_SUBTRACT_HANDLER_HPP_

#include "async_readers.hpp"
#include "async_writers.hpp"
#include "bound_inbuf.hpp"
#include "bound_outbuf.hpp"
#include "linkage.h"
#include "logging_context.hpp"
#include "result.hpp"
#include "subroutine.hpp"

namespace cuti
{

/*
 * Simple 'subtract' method handler (for testing purposes)
 */
struct CUTI_ABI subtract_handler_t
{
  using result_value_t = void;

  subtract_handler_t(result_t<void>& result,
                     logging_context_t const& context,
                     bound_inbuf_t& inbuf,
                     bound_outbuf_t& outbuf);

  subtract_handler_t(subtract_handler_t const&) = delete;
  subtract_handler_t& operator=(subtract_handler_t const&) = delete;
  
  void start();

private :
  void on_first_arg(int arg);
  void on_second_arg(int arg);
  void on_done();

private :
  result_t<void>& result_;
  logging_context_t const& context_;
  subroutine_t<subtract_handler_t, reader_t<int>> int_reader_;
  subroutine_t<subtract_handler_t, writer_t<int>> int_writer_;

  int first_arg_;
};
  
} // cuti

#endif
