/*
 * Copyright (C) 2021-2022 CodeShop B.V.
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

#ifndef X264_LISTENER_HPP_
#define X264_LISTENER_HPP_

#include <cuti/listener.hpp>

#include <memory>

namespace cuti
{

struct endpoint_t;
struct logging_context_t;
struct tcp_acceptor_t;

} // cuti

struct x264_listener_t : cuti::listener_t
{
  x264_listener_t(cuti::logging_context_t const& context,
                  cuti::endpoint_t const& endpoint);

  cuti::cancellation_ticket_t call_when_ready(
    cuti::scheduler_t& scheduler, cuti::callback_t callback) override;
  std::unique_ptr<cuti::client_t> on_ready() override;

  ~x264_listener_t() override;
    
private :
  cuti::logging_context_t const& context_;
  std::unique_ptr<cuti::tcp_acceptor_t> acceptor_;
};
  
#endif
