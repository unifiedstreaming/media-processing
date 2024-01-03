/*
 * Copyright (C) 2022-2024 CodeShop B.V.
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

#include "method_runner.hpp"

#include "bound_inbuf.hpp"
#include "method.hpp"
#include "method_map.hpp"
#include "parse_error.hpp"

namespace cuti
{

method_runner_t::method_runner_t(result_t<void>& result,
                                 logging_context_t const& context,
                                 bound_inbuf_t& inbuf,
                                 bound_outbuf_t& outbuf,
                                 method_map_t const& map)
: result_(result)
, context_(context)
, inbuf_(inbuf)
, outbuf_(outbuf)
, map_(map)
, method_(nullptr)
{ }

void method_runner_t::start(identifier_t const& name)
{
  method_ = map_.create_method_instance(
    name, result_, context_, inbuf_, outbuf_);
  if(method_ == nullptr)
  {
    result_.fail(parse_error_t("method not found"));
    return;
  }

  method_->start();
}

method_runner_t::~method_runner_t()
{ }

} // cuti
