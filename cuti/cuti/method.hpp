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

#ifndef CUTI_METHOD_HPP_
#define CUTI_METHOD_HPP_

#include "linkage.h"
#include "result.hpp"
#include "stack_marker.hpp"

#include <memory>
#include <utility>

namespace cuti
{

struct logging_context_t;
struct bound_inbuf_t;
struct bound_outbuf_t;

/*
 * Interface type for an asynchronous method instance.
 */
struct CUTI_ABI method_t
{
  using result_value_t = void;

  method_t()
  { }

  method_t(method_t const&) = delete;
  method_t& operator=(method_t const&) = delete;

  virtual void start(stack_marker_t& base_marker) = 0;

  virtual ~method_t()
  { }
};

/*
 * Concrete asynchronous method instance type delegating to
 * asynchronous routine type Impl.
 */
template<typename Impl>
struct method_inst_t : method_t
{
  template<typename... Others>
  method_inst_t(result_t<void>& result,
                logging_context_t const& context,
                bound_inbuf_t& inbuf,
                bound_outbuf_t& outbuf,
                Others&&... others)
  : impl_(result, context, inbuf, outbuf, std::forward<Others>(others)...)
  { }

  void start(stack_marker_t& base_marker) override
  {
    impl_.start(base_marker);
  }

private :
  Impl impl_;
};

/*
 * Convenience function for creating a method instance.
 */
template<typename Impl, typename... Others>
std::unique_ptr<method_t>
make_method(result_t<void>& result,
            logging_context_t const& context,
            bound_inbuf_t& inbuf,
            bound_outbuf_t& outbuf,
            Others&&... others)
{
  return std::make_unique<method_inst_t<Impl>>(
    result, context, inbuf, outbuf, std::forward<Others>(others)...);
}

} // cuti

#endif
