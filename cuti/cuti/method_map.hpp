/*
 * Copyright (C) 2021-2023 CodeShop B.V.
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
#include "method.hpp"

#include <cassert>
#include <map>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

namespace cuti
{

/*
 * Factory creating method instances by name.
 */
struct CUTI_ABI method_map_t
{
  method_map_t()
  : factories_()
  { }

  method_map_t(method_map_t const&) = delete;
  method_map_t& operator=(method_map_t const&) = delete;
  
  /*
   * Adds a method factory for the method named name.
   */
  template<typename MethodFactory>
  void add_method_factory(std::string name, MethodFactory&& method_factory)
  {
    bool inserted;
    std::tie(std::ignore, inserted) = factories_.insert(
      std::make_pair(
        std::move(name),
        make_factory_wrapper(std::forward<MethodFactory>(method_factory))));
    assert(inserted);
  }

  /*
   * Creates a method instance for the method named name, returning
   * nullptr if name is not found.
   */
  std::unique_ptr<method_t>
  create_method_instance(identifier_t const& name,
                         result_t<void>& result,
                         logging_context_t const& context,
                         bound_inbuf_t& inbuf,
                         bound_outbuf_t& outbuf) const
  {
    std::unique_ptr<method_t> method = nullptr;

    auto pos = factories_.find(name);
    if(pos != factories_.end())
    {
      method = (*pos->second)(result, context, inbuf, outbuf);
    }

    return method;
  }
    
private :
  struct factory_wrapper_t
  {
    factory_wrapper_t()
    { }

    factory_wrapper_t(factory_wrapper_t const&) = delete;
    factory_wrapper_t& operator=(factory_wrapper_t const&) = delete;

    virtual std::unique_ptr<method_t>
    operator()(result_t<void>& result,
               logging_context_t const& context,
               bound_inbuf_t& inbuf,
               bound_outbuf_t& outbuf) const = 0;

    virtual ~factory_wrapper_t()
    { }
  };

  template<typename Factory>
  struct factory_wrapper_inst_t : factory_wrapper_t
  {
    template<typename FFactory>
    factory_wrapper_inst_t(FFactory&& wrapped)
    : wrapped_(std::forward<FFactory>(wrapped))
    {
      if constexpr(std::is_pointer_v<Factory>)
      {
        assert(wrapped_ != nullptr);
      }
    }

    std::unique_ptr<method_t>
    operator()(result_t<void>& result,
               logging_context_t const& context,
               bound_inbuf_t& inbuf,
               bound_outbuf_t& outbuf) const override
    {
      return wrapped_(result, context, inbuf, outbuf);
    }

  private :
    Factory wrapped_;
  };

  template<typename Factory>
  static std::unique_ptr<factory_wrapper_t const>
  make_factory_wrapper(Factory&& wrapped)
  {
    return std::make_unique<factory_wrapper_inst_t<std::decay_t<Factory>>>(
      std::forward<Factory>(wrapped));
  }
    
private :
  std::map<identifier_t, std::unique_ptr<factory_wrapper_t const>> factories_;
};

template<typename Impl>
auto default_method_factory()
{
  return [](result_t<void>& result,
            logging_context_t const& context,
            bound_inbuf_t& inbuf,
            bound_outbuf_t& outbuf)
  {
    return make_method<Impl>(result, context, inbuf, outbuf);
  };
}

} // cuti

#endif
