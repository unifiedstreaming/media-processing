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

#ifndef CUTI_METHOD_MAP_HPP_
#define CUTI_METHOD_MAP_HPP_

#include "identifier.hpp"
#include "linkage.h"
#include "method_handler.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <memory>
#include <utility>

namespace cuti
{

struct CUTI_ABI method_map_t
{
  struct CUTI_ABI entry_t
  {
    template<typename HandlerFactory>
    entry_t(std::string method, HandlerFactory f)
    : method_(std::move(method))
    , factory_wrapper_(std::make_unique<
        factory_wrapper_instance_t<HandlerFactory>>(std::move(f)))
    {
      assert(method_.is_valid());
    }

    identifier_t const& method() const
    {
      return method_;
    }

    std::unique_ptr<method_handler_t> make_method_handler(
      logging_context_t& context,
      result_t<void>& result,
      bound_inbuf_t& inbuf,
      bound_outbuf_t& outbuf) const
    {
      return (*factory_wrapper_)(context, result, inbuf, outbuf);
    }
    
  private :
    struct CUTI_ABI factory_wrapper_t
    {
      virtual std::unique_ptr<method_handler_t> operator()(
        logging_context_t& context,
        result_t<void>& result,
        bound_inbuf_t& inbuf,
        bound_outbuf_t& outbuf) const = 0;

      virtual ~factory_wrapper_t()
      { }
    };

    template<typename HandlerFactory>
    struct factory_wrapper_instance_t : factory_wrapper_t
    {
      factory_wrapper_instance_t(HandlerFactory f)
      : f_(std::move(f))
      {
        if constexpr(std::is_pointer_v<HandlerFactory>)
        {
          assert(f_ != nullptr);
        }
      }

      std::unique_ptr<method_handler_t> operator()(
        logging_context_t& context,
        result_t<void>& result,
        bound_inbuf_t& inbuf,
        bound_outbuf_t& outbuf) const override
      {
        return f_(context, result, inbuf, outbuf);
      }

    private:
      HandlerFactory f_;
    };

  private :
    identifier_t method_;
    std::unique_ptr<factory_wrapper_t> factory_wrapper_;
  };

  method_map_t(entry_t const* first, entry_t const* last)
  : first_(first)
  , last_(last)
  {
    auto not_less = [](entry_t const& e1, entry_t const& e2)
    { return !(e1.method() < e2.method()); };

    static_cast<void>(not_less);
    assert(std::adjacent_find(first_, last_, not_less) == last_);
  }

  entry_t const* find_entry(identifier_t const& method) const
  {
    auto method_less = [](entry_t const& entry, identifier_t const& method)
    { return entry.method() < method; };

    auto pos = std::lower_bound(first_, last_, method, method_less);
    if(pos == last_ || method < pos->method())
    {
      return nullptr;
    }

    return &*pos;
  }
  
private :
  entry_t const* first_;
  entry_t const* last_;
};
  
} // cuti

#endif
