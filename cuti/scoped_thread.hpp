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

#ifndef XES_SCOPED_THREAD_HPP_
#define XES_SCOPED_THREAD_HPP_

#include <functional>
#include <thread>

namespace xes
{

struct scoped_thread_t
{
  explicit scoped_thread_t(std::function<void()> f);

  scoped_thread_t(scoped_thread_t const&) = delete;
  scoped_thread_t& operator=(scoped_thread_t const&) = delete;
  
  ~scoped_thread_t();

private :
  std::thread thread_;
};

} // namespace xes

#endif
