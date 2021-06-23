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

#ifndef CUTI_SCOPED_THREAD_HPP_
#define CUTI_SCOPED_THREAD_HPP_

#include "linkage.h"

#include <functional>
#include <thread>

/*
 * This is a wrapper for std::thread, that automatically joins the thread upon
 * destruction.
 */

namespace cuti
{

struct CUTI_ABI scoped_thread_t
{
  explicit scoped_thread_t(std::function<void()> f);

  scoped_thread_t(scoped_thread_t const&) = delete;
  scoped_thread_t& operator=(scoped_thread_t const&) = delete;

  ~scoped_thread_t();

private :
  std::thread thread_;
};

} // namespace cuti

#endif
