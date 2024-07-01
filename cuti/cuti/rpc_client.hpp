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

#ifndef CUTI_RPC_CLIENT_HPP_
#define CUTI_RPC_CLIENT_HPP_

#include "bound_inbuf.hpp"
#include "bound_outbuf.hpp"
#include "default_scheduler.hpp"
#include "endpoint.hpp"
#include "final_result.hpp"
#include "identifier.hpp"
#include "input_list.hpp"
#include "logging_context.hpp"
#include "linkage.h"
#include "nb_client.hpp"
#include "nb_client_cache.hpp"
#include "output_list.hpp"
#include "rpc_engine.hpp"
#include "stack_marker.hpp"
#include "throughput_checker.hpp"
#include "type_list.hpp"

#include <cassert>
#include <cstddef>
#include <iosfwd>
#include <memory>
#include <utility>

namespace cuti
{

struct CUTI_ABI rpc_client_t
{
  rpc_client_t(logging_context_t const& context,
               nb_client_cache_t& client_cache,
               endpoint_t server_address,
               throughput_settings_t settings = throughput_settings_t());

  rpc_client_t(rpc_client_t const&) = delete;
  rpc_client_t& operator=(rpc_client_t const&) = delete;

  /*
   * Starts an RPC call.
   * PRE: !this->busy()
   */
  template<typename... InputArgs, typename... OutputArgs>
  void start(identifier_t method, 
             std::unique_ptr<input_list_t<InputArgs...>> inputs,
             std::unique_ptr<output_list_t<OutputArgs...>> outputs)
  {
    assert(!this->busy());

    assert(method.is_valid());
    assert(inputs != nullptr);
    assert(outputs != nullptr);

    curr_call_ = std::make_unique<
      call_inst_t<type_list_t<InputArgs...>, type_list_t<OutputArgs...>>>(
        context_, scheduler_, client_cache_, server_address_, settings_,
        std::move(method), std::move(inputs), std::move(outputs));
  }

  /*
   * Tells if there is a currently active RPC call.
   */
  bool busy() const
  { return curr_call_ != nullptr; }

  /*
   * Have the currently active RPC call make some progress; may throw
   * to report errors detected by the RPC engine.
   * PRE: this->busy().
   */
  void step();

  /*
   * Completes any currently running call.
   * POST: !this->busy()
   */
  void complete_current_call()
  {
    while(this->busy())
    {
      this->step();
    }
  }

  /*
   * Performs a full RPC call.
   * PRE: !this->busy()
   */
  template<typename... InputArgs, typename... OutputArgs>
  void operator()(identifier_t method,
                  std::unique_ptr<input_list_t<InputArgs...>> inputs,
                  std::unique_ptr<output_list_t<OutputArgs...>> outputs)
  {
    this->start(std::move(method), std::move(inputs), std::move(outputs));
    this->complete_current_call();
  }

private :
  struct CUTI_ABI call_t
  {
    explicit call_t(logging_context_t const& context,
                    default_scheduler_t& scheduler,
                    nb_client_cache_t& client_cache,
                    endpoint_t const& server_address);

    call_t(call_t const&) = delete;
    call_t& operator=(call_t const&) = delete;

    bool busy() const
    { return !done_; }

    // step() throws on exceptions detected by the RPC engine
    // PRE: this->busy();
    void step();

    virtual ~call_t() = 0;

  protected :
    result_t<void>& result()
    { return result_; }

    nb_inbuf_t& nb_inbuf()
    { return nb_client_->nb_inbuf(); }

    nb_outbuf_t& nb_outbuf()
    { return nb_client_->nb_outbuf(); }

  private :
    logging_context_t const& context_;
    default_scheduler_t& scheduler_;
    final_result_t<void> result_;
    bool done_;    
    nb_client_cache_t& client_cache_;
    std::unique_ptr<nb_client_t> nb_client_;
  };

  template<typename ReplyTypes, typename RequestTypes>
  struct call_inst_t : call_t
  {
    using input_list_ptr_t = std::unique_ptr<
      bind_to_type_list_t<input_list_t, ReplyTypes>>;
    using output_list_ptr_t = std::unique_ptr<
      bind_to_type_list_t<output_list_t, RequestTypes>>;

    call_inst_t(logging_context_t const& context,
                default_scheduler_t& scheduler,
                nb_client_cache_t& client_cache,
                endpoint_t const& server_address,
                throughput_settings_t settings,
                identifier_t method,
                input_list_ptr_t inputs,
                output_list_ptr_t outputs)   
    : call_t(context, scheduler, client_cache, server_address)
    , engine_(call_t::result(), scheduler,
        call_t::nb_inbuf(), call_t::nb_outbuf(), std::move(settings))
    {
      assert(method.is_valid());
      assert(inputs != nullptr);
      assert(outputs != nullptr);

      stack_marker_t base_marker;
      engine_.start(base_marker,
        std::move(method), std::move(inputs), std::move(outputs));
    }

  private :
    rpc_engine_t<ReplyTypes, RequestTypes> engine_;
  };
  
private :
  logging_context_t const& context_;
  default_scheduler_t scheduler_;
  nb_client_cache_t& client_cache_;
  endpoint_t server_address_;
  throughput_settings_t settings_;
  std::unique_ptr<call_t> curr_call_;
};

} // cuti

#endif
