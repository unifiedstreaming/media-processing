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

#include "request_handler.hpp"

#include <utility>

namespace cuti
{

request_handler_t::request_handler_t(
  result_t<void>& result,
  logging_context_t& context,
  bound_inbuf_t& inbuf,
  bound_outbuf_t& outbuf,
  method_map_t<request_handler_t> const& method_map)
: result_(result)
, context_(context)
, inbuf_(inbuf)
, outbuf_(outbuf)
, method_map_(method_map)
, method_reader_(*this, &request_handler_t::on_method_reader_failure, inbuf_)
, method_(nullptr)
, eom_checker_(*this, &request_handler_t::on_eom_checker_failure, inbuf_)
, exception_writer_(*this, result_, outbuf_)
, eom_writer_(*this, result_, outbuf_)
, request_drainer_(*this, result_, inbuf_)
{ }

void request_handler_t::start()
{
  method_reader_.start(&request_handler_t::start_method);
}

void request_handler_t::start_method(identifier_t name)
{
  method_ = method_map_.create_method_instance(
    name, *this, &request_handler_t::on_method_failure,
    context_, inbuf_, outbuf_);

  if(method_ == nullptr)
  {
    this->report_failure("bad_method",
      "method \'" + name.as_string() + "\' not found");
    return;
  }

  if(auto msg = context_.message_at(loglevel_t::info))
  {
    *msg << "request_handler: starting method " << name;
  }

  method_->start(&request_handler_t::on_method_succeeded);
}
  
void request_handler_t::on_method_succeeded()
{
  if(auto msg = context_.message_at(loglevel_t::info))
  {
    *msg << "request_handler: method succeeded";
  }

  eom_checker_.start(&request_handler_t::write_eom);
}

void request_handler_t::on_method_reader_failure(std::exception_ptr ex)
{
  this->report_failure("bad_request", std::move(ex));
}

void request_handler_t::on_method_failure(std::exception_ptr ex)
{
  this->report_failure("method_failed", std::move(ex));
}

void request_handler_t::on_eom_checker_failure(std::exception_ptr ex)
{
  this->report_failure("bad_request", std::move(ex));
}

void request_handler_t::report_failure(std::string type, std::exception_ptr ex)
{
  std::string description;
  try
  {
    std::rethrow_exception(std::move(ex));
  }
  catch(std::exception const& stdex)
  {
    description = stdex.what();
  }

  this->report_failure(std::move(type), std::move(description));
}

void request_handler_t::report_failure(std::string type,
                                       std::string description)
{
  remote_error_t error(type, description);

  if(auto msg = context_.message_at(loglevel_t::error))
  {
    *msg << "request_handler: reporting error: " << error;
  }

  exception_writer_.start(&request_handler_t::write_eom, std::move(error));
}

void request_handler_t::write_eom()
{
  eom_writer_.start(&request_handler_t::drain_request);
}

void request_handler_t::drain_request()
{
  request_drainer_.start(&request_handler_t::on_request_drained);
}

void request_handler_t::on_request_drained()
{
  result_.submit();
}

} // cuti