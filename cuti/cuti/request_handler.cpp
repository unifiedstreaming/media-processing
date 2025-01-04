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

#include "request_handler.hpp"

#include <utility>

namespace cuti
{

request_handler_t::request_handler_t(
  result_t<void>& result,
  logging_context_t const& context,
  bound_inbuf_t& inbuf,
  bound_outbuf_t& outbuf,
  method_map_t const& map)
: result_(result)
, context_(context)
, inbuf_(inbuf)
, method_reader_(*this, &request_handler_t::on_method_reader_failure, inbuf_)
, method_runner_(*this, &request_handler_t::on_method_failure,
   context_, inbuf_, outbuf, map)
, eom_checker_(*this, &request_handler_t::on_eom_checker_failure, inbuf_)
, exception_writer_(*this, result_, outbuf)
, eom_writer_(*this, result_, outbuf)
, request_drainer_(*this, result_, inbuf_)
, method_name_()
{ }

void request_handler_t::start(stack_marker_t& base_marker)
{
  method_name_.reset();
  method_reader_.start(base_marker, &request_handler_t::start_method);
}

void request_handler_t::start_method(
  stack_marker_t& base_marker, identifier_t name)
{
  assert(name.is_valid());
  method_name_.emplace(std::move(name));

  if(auto msg = context_.message_at(loglevel_t::info))
  {
    *msg << "request_handler " << inbuf_ << ": starting method \'" <<
      *method_name_ << "\'";
  }

  method_runner_.start(
    base_marker, &request_handler_t::on_method_succeeded, *method_name_);
}
  
void request_handler_t::on_method_succeeded(stack_marker_t& base_marker)
{
  assert(method_name_.has_value());

  if(auto msg = context_.message_at(loglevel_t::info))
  {
    *msg << "request_handler " << inbuf_ << ": method \'" <<
      *method_name_ << "\' succeeded";
  }

  eom_checker_.start(base_marker, &request_handler_t::write_eom);
}

void request_handler_t::on_method_reader_failure(
  stack_marker_t& base_marker, std::exception_ptr ex)
{
  this->report_failure(base_marker, "bad_request", std::move(ex));
}

void request_handler_t::on_method_failure(
  stack_marker_t& base_marker, std::exception_ptr ex)
{
  this->report_failure(base_marker, "method_failed", std::move(ex));
}

void request_handler_t::on_eom_checker_failure(
  stack_marker_t& base_marker, std::exception_ptr ex)
{
  this->report_failure(base_marker, "bad_request", std::move(ex));
}

void request_handler_t::report_failure(
  stack_marker_t& base_marker, std::string type, std::exception_ptr ex)
{
  std::string description;
  try
  {
    std::rethrow_exception(std::move(ex));
  }
  catch(std::exception const& stdex)
  {
    if(method_name_.has_value())
    {
      description += method_name_->as_string();
      description += ": ";
    }
    description += stdex.what();
  }

  remote_error_t error(type, description);

  if(auto msg = context_.message_at(loglevel_t::error))
  {
    *msg << "request_handler " << inbuf_ << ": reporting error: " << error;
  }

  exception_writer_.start(
    base_marker, &request_handler_t::write_eom, std::move(error));
}

void request_handler_t::write_eom(stack_marker_t& base_marker)
{
  eom_writer_.start(base_marker, &request_handler_t::drain_request);
}

void request_handler_t::drain_request(stack_marker_t& base_marker)
{
  request_drainer_.start(base_marker, &request_handler_t::on_request_drained);
}

void request_handler_t::on_request_drained(stack_marker_t& base_marker)
{
  result_.submit(base_marker);
}

} // cuti
