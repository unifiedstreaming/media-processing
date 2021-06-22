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

#include "x264_client.hpp"

#include "logging_context.hpp"
#include "tcp_connection.hpp"

x264_client_t::x264_client_t(cuti::logging_context_t const& context,
                             std::unique_ptr<cuti::tcp_connection_t> connection)
: context_(context)
, connection_(std::move(connection))
{
  connection_->set_nonblocking();
  if(auto msg = context.message_at(cuti::loglevel_t::info))
  {
    *msg << "accepted client " << *connection_;
  }
}

cuti::cancellation_ticket_t
x264_client_t::call_when_readable(cuti::scheduler_t& scheduler,
                                  cuti::callback_t callback)
{
  return connection_->call_when_readable(scheduler, std::move(callback));
}

bool
x264_client_t::on_readable()
{
    return false;
}

x264_client_t::~x264_client_t()
{
  if(auto msg = context_.message_at(cuti::loglevel_t::info))
  {
    *msg << "disconnecting client " << *connection_;
  }
}
