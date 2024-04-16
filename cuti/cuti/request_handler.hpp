/*
 * Copyright (C) 2021-2024 CodeShop B.V.
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

#ifndef CUTI_REQUEST_HANDLER_HPP_
#define CUTI_REQUEST_HANDLER_HPP_

#include "async_readers.hpp"
#include "async_writers.hpp"
#include "bound_inbuf.hpp"
#include "bound_outbuf.hpp"
#include "identifier.hpp"
#include "linkage.h"
#include "logging_context.hpp"
#include "method.hpp"
#include "method_map.hpp"
#include "method_runner.hpp"
#include "result.hpp"
#include "stack_marker.hpp"
#include "subroutine.hpp"

#include <exception>
#include <optional>
#include <string>

namespace cuti
{

struct CUTI_ABI request_handler_t
{
  using result_value_t = void;

  request_handler_t(result_t<void>& result,
                    logging_context_t const& context,
                    bound_inbuf_t& inbuf,
                    bound_outbuf_t& outbuf,
                    method_map_t const& map);

  request_handler_t(request_handler_t const&) = delete;
  request_handler_t& operator=(request_handler_t const&) = delete;
  
  void start(stack_marker_t& base_marker);

private :
  void start_method(stack_marker_t& base_marker, identifier_t name);
  void on_method_succeeded(stack_marker_t& base_marker);

  void on_method_reader_failure(
    stack_marker_t& base_marker, std::exception_ptr ex);
  void on_method_failure(
    stack_marker_t& base_marker, std::exception_ptr ex);
  void on_eom_checker_failure(
    stack_marker_t& base_marker, std::exception_ptr ex);

  void report_failure(
    stack_marker_t& base_marker, std::string type, std::exception_ptr ex);

  void write_eom(stack_marker_t& base_marker);
  void drain_request(stack_marker_t& base_marker);
  void on_request_drained(stack_marker_t& base_marker);

private :
  result_t<void>& result_;
  logging_context_t const& context_;
  bound_inbuf_t& inbuf_;

  subroutine_t<request_handler_t, reader_t<identifier_t>,
    failure_mode_t::handle_in_parent> method_reader_;
  subroutine_t<request_handler_t, method_runner_t,
    failure_mode_t::handle_in_parent> method_runner_;
  subroutine_t<request_handler_t, eom_checker_t,
    failure_mode_t::handle_in_parent> eom_checker_;
  subroutine_t<request_handler_t, exception_writer_t> exception_writer_;
  subroutine_t<request_handler_t, eom_writer_t> eom_writer_;
  subroutine_t<request_handler_t, message_drainer_t> request_drainer_;

  std::optional<identifier_t> method_name_;
};

} // cuti

#endif
