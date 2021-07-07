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

#ifndef CUTI_ASYNC_INPUT_HPP_
#define CUTI_ASYNC_INPUT_HPP_

#include "callback.hpp"
#include "linkage.h"

namespace cuti
{

struct scheduler_t;

/*
 * Asynchronous input adapter interface.
 */
struct CUTI_ABI async_input_t
{
  async_input_t();

  async_input_t(async_input_t const&) = delete;
  async_input_t& operator=(async_input_t const&) = delete;

  virtual void
  call_when_readable(scheduler_t& scheduler, callback_t callback) = 0;

  virtual void
  cancel_when_readable() noexcept = 0;
  
  virtual char*
  read(char* first, char const* last) = 0;

  virtual int
  error_status() const noexcept = 0;

  virtual ~async_input_t();
};
  
} // cuti

#endif
