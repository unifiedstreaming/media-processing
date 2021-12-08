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

#ifndef CUTI_METHOD_HANDLER_HPP_
#define CUTI_METHOD_HANDLER_HPP_

#include "bound_inbuf.hpp"
#include "bound_outbuf.hpp"
#include "linkage.h"
#include "logging_context.hpp"
#include "result.hpp"

#include <memory>

namespace cuti
{

/*
 * RPC method handler interface
 */
struct CUTI_ABI method_handler_t
{
  method_handler_t()
  { }

  method_handler_t(method_handler_t const&);
  method_handler_t& operator=(method_handler_t const&);

  virtual void start() = 0;

  virtual ~method_handler_t()
  { }
};

/*
 * RPC method handler instance type
 */
template<typename T>
struct method_handler_instance_t : method_handler_t
{
  method_handler_instance_t(result_t<void>& result,
                            logging_context_t& context,
                            bound_inbuf_t& inbuf,
                            bound_outbuf_t& outbuf)
  : delegate_(result, context, inbuf, outbuf)
  { }

  void start() override
  {
    delegate_.start();
  }

private :
  T delegate_;
};

/*
 * RPC method handler factory function
 */
template<typename T>
std::unique_ptr<method_handler_t>
make_method_handler(result_t<void>& result,
                    logging_context_t& context,
                    bound_inbuf_t& inbuf,
                    bound_outbuf_t& outbuf)
{
  return std::make_unique<method_handler_instance_t<T>>(
    result, context, inbuf, outbuf);
}

} // cuti

#endif
