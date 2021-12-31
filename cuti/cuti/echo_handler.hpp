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

#ifndef CUTI_ECHO_HANDLER_HPP_
#define CUTI_ECHO_HANDLER_HPP_

#include "async_readers.hpp"
#include "async_writers.hpp"
#include "bound_inbuf.hpp"
#include "bound_outbuf.hpp"
#include "linkage.h"
#include "logging_context.hpp"
#include "result.hpp"
#include "subroutine.hpp"

#include <optional>
#include <string>

namespace cuti
{

/*
 * Streaming handler echoing a sequence of strings (for testing purposes)
 */
struct CUTI_ABI echo_handler_t
{
  using result_value_t = void;

  echo_handler_t(result_t<void>& result,
                 logging_context_t& context,
                 bound_inbuf_t& inbuf,
                 bound_outbuf_t& outbuf,
                 std::optional<std::string> censored = std::nullopt);

  echo_handler_t(echo_handler_t const&) = delete;
  echo_handler_t& operator=(echo_handler_t const&) = delete;
  
  void start();

private :
  void write_begin();
  void echo_elements();
  void on_end_checker(bool at_end);
  void on_end_written();
  void write_element(std::string value);

private :
  result_t<void>& result_;
  logging_context_t& context_;
  std::optional<std::string> censored_;

  subroutine_t<echo_handler_t, begin_sequence_reader_t> begin_reader_;
  subroutine_t<echo_handler_t, begin_sequence_writer_t> begin_writer_;

  subroutine_t<echo_handler_t, end_sequence_checker_t> end_checker_;
  subroutine_t<echo_handler_t, end_sequence_writer_t> end_writer_;

  subroutine_t<echo_handler_t, reader_t<std::string>> element_reader_;
  subroutine_t<echo_handler_t, writer_t<std::string>> element_writer_;
};

} // cuti

#endif
