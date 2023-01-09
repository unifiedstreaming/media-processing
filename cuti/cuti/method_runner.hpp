/*
 * Copyright (C) 2022-2023 CodeShop B.V.
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

#ifndef CUTI_METHOD_RUNNER_HPP_
#define CUTI_METHOD_RUNNER_HPP_

#include "linkage.h"
#include "result.hpp"

#include <memory>

namespace cuti
{

struct bound_inbuf_t;
struct bound_outbuf_t;
struct identifier_t;
struct logging_context_t;
struct method_t;
struct method_map_t;

struct CUTI_ABI method_runner_t
{
  using result_value_t = void;

  method_runner_t(result_t<void>& result,
                  logging_context_t const& context,
                  bound_inbuf_t& inbuf,
                  bound_outbuf_t& outbuf,
                  method_map_t const& map);

  method_runner_t(method_runner_t const&) = delete;
  method_runner_t& operator=(method_runner_t const&) = delete;
  
  void start(identifier_t const& name);

  ~method_runner_t();

private :
  result_t<void>& result_;
  logging_context_t const& context_;
  bound_inbuf_t& inbuf_;
  bound_outbuf_t& outbuf_;
  method_map_t const& map_;
  std::unique_ptr<method_t> method_;
};

} // cuti

#endif
