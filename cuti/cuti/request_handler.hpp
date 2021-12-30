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
#include "result.hpp"
#include "subroutine.hpp"

#include <exception>
#include <string>

namespace cuti
{

struct CUTI_ABI request_handler_t
{
  using result_value_t = void;

  request_handler_t(result_t<void>& result,
                    logging_context_t& context,
                    bound_inbuf_t& inbuf,
                    bound_outbuf_t& outbuf,
                    method_map_t<request_handler_t> const& method_map);

  request_handler_t(request_handler_t const&) = delete;
  request_handler_t& operator=(request_handler_t const&) = delete;
  
  void start();

private :
  void start_method(identifier_t name);
  void on_method_succeeded();

  void on_method_reader_failure(std::exception_ptr ex);
  void on_method_failure(std::exception_ptr ex);
  void on_eom_checker_failure(std::exception_ptr ex);

  void report_failure(std::string type, std::exception_ptr ex);
  void report_failure(std::string type, std::string description);

  void write_eom();
  void drain_request();
  void on_request_drained();

private :
  result_t<void>& result_;
  logging_context_t& context_;
  bound_inbuf_t& inbuf_;
  bound_outbuf_t& outbuf_;
  method_map_t<request_handler_t> const& method_map_;

  subroutine_t<request_handler_t, reader_t<identifier_t>,
    failure_mode_t::handle_in_parent> method_reader_;
  std::unique_ptr<method_t<request_handler_t>> method_;
  subroutine_t<request_handler_t, eom_checker_t,
    failure_mode_t::handle_in_parent> eom_checker_;
  subroutine_t<request_handler_t, exception_writer_t> exception_writer_;
  subroutine_t<request_handler_t, eom_writer_t> eom_writer_;
  subroutine_t<request_handler_t, message_drainer_t> request_drainer_;
};

} // cuti

#endif
