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

#ifndef XES_EXCEPTION_BUILDER_HPP_
#define XES_EXCEPTION_BUILDER_HPP_

#include <ostream>
#include <string>
#include <utility>

#include "membuf.hpp"

namespace xes
{

template<typename T>
struct exception_builder_t : std::ostream
{
  exception_builder_t()
  : std::ostream(nullptr)
  , buf_()
  { this->rdbuf(&buf_); }

  exception_builder_t(exception_builder_t const&) = delete;
  exception_builder_t& operator=(exception_builder_t const&) = delete;

  template<typename... Args>
  void explode(Args&&... args) const
  {
    throw T(std::string(buf_.begin(), buf_.end()),
            std::forward<Args>(args)...);
  }

private :
  membuf_t buf_;
};

} // xes

#endif
