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

#ifndef CUTI_REQUEST_HANDLER_HPP_
#define CUTI_REQUEST_HANDLER_HPP_

#include "bound_inbuf.hpp"
#include "bound_outbuf.hpp"
#include "identifier.hpp"
#include "linkage.h"
#include "logging_context.hpp"
#include "result.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <memory>

namespace cuti
{

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

struct CUTI_ABI method_entry_t
{
  template<typename F>
  method_entry_t(std::string method, F f)
  : method_(std::move(method))
  , factory_(std::make_unique<factory_instance_t<F>>(std::move(f)))
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
    return (*factory_)(context, result, inbuf, outbuf);
  }
    
private :
  struct factory_impl_t
  {
    factory_impl_t()
    { }

    virtual std::unique_ptr<method_handler_t> operator()(
      logging_context_t& context,
      result_t<void>& result,
      bound_inbuf_t& inbuf,
      bound_outbuf_t& outbuf) const = 0;

    virtual ~factory_impl_t()
    { }
  };

  template<typename F>
  struct factory_instance_t : factory_impl_t
  {
    factory_instance_t(F f)
    : f_(f)
    {
      if constexpr(std::is_pointer_v<F>)
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
    F f_;
  };

private :
  identifier_t method_;
  std::unique_ptr<factory_impl_t> factory_;
};

struct CUTI_ABI method_map_t
{
  template<std::size_t N>
  method_map_t(method_entry_t const (&entries)[N])
  : first_(entries)
  , last_(entries + N)
  {
    auto not_sorted = [](method_entry_t const& e1, method_entry_t const& e2)
    { return e1.method() >= e2.method(); };
    
    assert(std::adjacent_find(first_, last_, not_sorted) == last_);
  }

  method_entry_t const* find_method_entry(identifier_t const& method) const
  {
    auto method_less = [](method_entry_t const& entry,
                          identifier_t const& method)
    { return entry.method() < method; };

    auto pos = std::lower_bound(first_, last_, method, method_less);
    if(pos == last_ || pos->method() != method)
    {
      return nullptr;
    }

    return &*pos;
  }
  
private :
  method_entry_t const* first_;
  method_entry_t const* last_;
};
  
} // cuti

#endif
