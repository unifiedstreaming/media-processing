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

#ifndef CUTI_ASYNC_TCP_OUTPUT_ADAPTER_HPP_
#define CUTI_ASYNC_TCP_OUTPUT_ADAPTER_HPP_

#include "async_outbuf.hpp"
#include "ticket_holder.hpp"

#include <memory>

namespace cuti
{

struct tcp_connection_t;

/*
 * Asynchronous tcp output adapter.
 */
struct CUTI_ABI async_tcp_output_adapter_t : async_output_adapter_t
{
  explicit async_tcp_output_adapter_t(std::shared_ptr<tcp_connection_t> conn);

  void call_when_writable(scheduler_t& scheduler,
                          callback_t callback) override;

  void cancel_when_writable() noexcept override;

  char const* write(char const* first, char const* last) override;

  int error_status() const noexcept override;

private :
  std::shared_ptr<tcp_connection_t> conn_;
  int error_status_;
  ticket_holder_t writable_holder_;
};

} // cuti

#endif