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
#include <type_traits>
#include <utility>

#include "membuf.hpp"

namespace cuti
{

/*
 * The default exception factory type used by exception_builder_t
 * assumes it can construct an exception object by passing a
 * string with the error message as the only argument to the
 * exception object's constructor.
 */
template<typename T>
struct default_exception_factory_t
{
  T operator()(std::string const& msg) const
  { return T{msg}; }
};
  
template<typename T,
         typename ExceptionFactory = default_exception_factory_t<T>>
struct exception_builder_t : std::ostream
{
  exception_builder_t()
  : std::ostream(nullptr)
  , buf_()
  , exception_factory_()
  { this->rdbuf(&buf_); }
  
  template<typename A1, typename = std::enable_if_t<
    !std::is_convertible_v<std::decay_t<A1>*, exception_builder_t const*>>>
  explicit exception_builder_t(A1&& a1)
  : std::ostream(nullptr)
  , buf_()
  , exception_factory_(std::forward<A1>(a1))
  { this->rdbuf(&buf_); }

  template<typename A1, typename A2, typename... An>
  exception_builder_t(A1&& a1, A2&& a2, An&&... an)
  : std::ostream(nullptr)
  , buf_()
  , exception_factory_(std::forward<A1>(a1),
                       std::forward<A2>(a2),
                       std::forward<An>(an)...)
  { this->rdbuf(&buf_); }

  exception_builder_t(exception_builder_t const&) = delete;
  exception_builder_t& operator=(exception_builder_t const&) = delete;

  T exception_object() const
  {
    return exception_factory_(std::string(buf_.begin(), buf_.end()));
  }

  std::exception_ptr exception_ptr() const
  {
    return std::make_exception_ptr(this->exception_object());
  }

  [[noreturn]] void explode() const
  {
    throw this->exception_object();
  }

private :
  membuf_t buf_;
  ExceptionFactory exception_factory_;
};

} // cuti

#endif
