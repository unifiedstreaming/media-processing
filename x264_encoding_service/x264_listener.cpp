/*
 * Copyright (C) 2021 CodeShop B.V.
 *
 * This file is part of the x264_encoding_service.
 *
 * The x264_encoding_service is free software: you can redistribute it
 * and/or modify it under the terms of version 2 of the GNU General
 * Public License as published by the Free Software Foundation.
 *
 * The x264_encoding_service is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See version 2 of the GNU General Public License for more details.
 *
 * You should have received a copy of version 2 of the GNU General
 * Public License along with the x264_encoding_service.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "x264_listener.hpp"

#include "x264_client.hpp"

#include "logging_context.hpp"
#include "system_error.hpp"
#include "tcp_acceptor.hpp"

x264_listener_t::x264_listener_t(cuti::logging_context_t const& context,
                                 cuti::endpoint_t const& endpoint)
: context_(context)
, acceptor_(std::make_unique<cuti::tcp_acceptor_t>(endpoint))
{
  acceptor_->set_nonblocking();

  if(auto msg = context.message_at(cuti::loglevel_t::info))
  {
      *msg << "listening at endpoint " << *acceptor_;
  }
}

cuti::cancellation_ticket_t
x264_listener_t::call_when_ready(cuti::scheduler_t& scheduler,
                                 cuti::callback_t callback)
{
  return acceptor_->call_when_ready(scheduler, std::move(callback));
}

std::unique_ptr<cuti::client_t>
x264_listener_t::on_ready()
{
  std::unique_ptr<cuti::tcp_connection_t> accepted;
  std::unique_ptr<cuti::client_t> result;

  int error = acceptor_->accept(accepted);
  if(error != 0)
  {
    if(auto msg = context_.message_at(cuti::loglevel_t::error))
    {
      *msg << "listener " << *acceptor_ << ": accept() failure: " <<
        cuti::system_error_string(error);
    }
  }
  else if(accepted == nullptr)
  {
    if(auto msg = context_.message_at(cuti::loglevel_t::warning))
    {
      *msg << "listener " << *acceptor_ << ": accept() would block";
    }
  }
  else
  {
    result = std::make_unique<x264_client_t>(context_, std::move(accepted));
  }

  return result;
}
  
x264_listener_t::~x264_listener_t()
{
}
