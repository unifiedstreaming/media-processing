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

#ifndef CUTI_ASYNC_OUTPUT_HPP_
#define CUTI_ASYNC_OUTPUT_HPP_

#include "callback.hpp"
#include "linkage.h"

namespace cuti
{

struct scheduler_t;

/*
 * Asynchronous output adapter interface.
 */
struct CUTI_ABI async_output_t
{
  async_output_t();

  async_output_t(async_output_t const&) = delete;
  async_output_t& operator=(async_output_t const&) = delete;

  virtual void
  call_when_writable(scheduler_t& scheduler, callback_t callback) = 0;

  virtual void
  cancel_when_writable() noexcept = 0;

  virtual char const*
  write(char const* first, char const* last) = 0;

  virtual int
  error_status() const noexcept = 0;

  virtual ~async_output_t();
};
  
} // cuti

#endif
