/*
 * Copyright (C) 2021-2026 CodeShop B.V.
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

#ifndef CUTI_EXCEPTION_BUILDER_HPP_
#define CUTI_EXCEPTION_BUILDER_HPP_

#include <ostream>
#include <exception>
#include <string>
#include <utility>

#include "membuf.hpp"

namespace cuti
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
  T exception_object(Args&&... args) const
  {
    return T(std::string(buf_.begin(), buf_.end()),
             std::forward<Args>(args)...);
  }

  template<typename... Args>
  std::exception_ptr exception_ptr(Args&&... args) const
  {
    return std::make_exception_ptr(
      this->exception_object(std::forward<Args>(args)...));
  }

  template<typename... Args>
  [[noreturn]] void explode(Args&&... args) const
  {
    throw this->exception_object(std::forward<Args>(args)...);
  }

private :
  membuf_t buf_;
};

} // cuti

#endif
