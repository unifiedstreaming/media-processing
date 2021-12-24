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

#ifndef CUTI_OUTPUT_LIST_HPP_
#define CUTI_OUTPUT_LIST_HPP_

#include "linkage.h"
#include "streaming_tag.hpp"
#include "type_list.hpp"

#include <optional>
#include <type_traits>

namespace cuti
{

/*
 * Abstract base interface for obtaining an output of type Value.
 */
template<typename Value>
struct output_t
{
  output_t()
  { }

  output_t(output_t const&) = delete;
  output_t& operator=(output_t const&) = delete;

  virtual Value get() = 0;

  virtual ~output_t()
  { }
};

/*
 * Abstract base interface for obtaining a stream of outputs of type
 * Value.  The end of the stream is marked by an empty optional.
 */
template<typename Value>
struct output_t<streaming_tag_t<Value>>
{
  output_t()
  { }

  output_t(output_t const&) = delete;
  output_t& operator=(output_t const&) = delete;

  virtual std::optional<Value> get() = 0;

  virtual ~output_t()
  { }
};

/*
 * Template for implementing output_t<Value>
 */
template<typename Value, typename Producer>
struct output_impl_t : output_t<Value>
{
  template<typename PProducer>
  explicit output_impl_t(PProducer&& producer)
  : producer_(std::forward<PProducer>(producer))
  { }

  Value get() override
  {
    if constexpr(std::is_convertible_v<Producer, Value>)
    {
      // assume a value was captured
      return std::move(producer_);
    }
    else
    {
      // assume a callable was captured
      return producer_();
    }
  }

private :
  Producer producer_;
};

/*
 * Template for implementing output_t<streaming_tag_t<Value>>.
 */
template<typename Value, typename Producer>
struct output_impl_t<streaming_tag_t<Value>, Producer>
: output_t<streaming_tag_t<Value>>
{
  template<typename PProducer>
  explicit output_impl_t(PProducer&& producer)
  : producer_(std::forward<PProducer>(producer))
  { }

  std::optional<Value> get() override
  {
    // always assume a callable was captured
    return producer_();
  }

private :
  Producer producer_;
};

/*
 * Abstract base interface for a list of outputs.  Each output has its
 * own value type.
 */
template<typename... Values>
struct output_list_t;

/*
 * Nothing to see here if there are no outputs.  Keep calm and carry
 * on.
 */
template<>
struct CUTI_ABI output_list_t<>
{
  output_list_t()
  { }

  output_list_t(output_list_t const&) = delete;
  output_list_t& operator=(output_list_t const&) = delete;

  virtual ~output_list_t()
  { }
};

/*
 * Otherwise, we require an accessor to get at the first output, and
 * another accessor to get at the other outputs.
 */
template<typename FirstValue, typename... OtherValues>
struct output_list_t<FirstValue, OtherValues...>
{
  output_list_t()
  { }

  output_list_t(output_list_t const&) = delete;
  output_list_t& operator=(output_list_t const&) = delete;

  virtual output_t<FirstValue>& first() = 0;
  virtual output_list_t<OtherValues...>& others() = 0;
  
  virtual ~output_list_t()
  { }
};

/*
 * Template for implementing an output_list_t for the value types in a
 * type list.  Each value type has a corresponding Producer type.
 */
template<typename Values, typename Producers>
struct output_list_impl_t;

/*
 * If there are no value/producer types, we have nothing to do to
 * implement our base interface.
 */
template<>
struct CUTI_ABI output_list_impl_t<type_list_t<>, type_list_t<>>
: output_list_t<>
{ };

/*
 * Otherwise, we implement our base interface by instantiating the
 * first output by its value/producer type, and use recursive
 * instantiation for the other value/producer types.
 */
template<typename FirstValue, typename... OtherValues,
         typename FirstProducer, typename... OtherProducers>
struct output_list_impl_t<type_list_t<FirstValue, OtherValues...>,
                          type_list_t<FirstProducer, OtherProducers...>>
: output_list_t<FirstValue, OtherValues...>
{
  template<typename FFirstProducer, typename... OOtherProducers>
  output_list_impl_t(FFirstProducer&& first_producer,
                     OOtherProducers&&... other_producers)
  : first_(std::forward<FFirstProducer>(first_producer))
  , others_(std::forward<OOtherProducers>(other_producers)...)
  { }

  output_t<FirstValue>& first() override
  {
    return first_;
  }

  output_list_t<OtherValues...>& others() override
  {
    return others_;
  }

private :
  output_impl_t<FirstValue, FirstProducer> first_;
  output_list_impl_t<type_list_t<OtherValues...>,
                     type_list_t<OtherProducers...>> others_;
};

/*
 * Helper functor template for building an output list.
 */
template<typename... Values>
struct make_output_list_t
{
  template<typename... Producers>
  auto operator()(Producers&&... producers) const
  {
    return output_list_impl_t<type_list_t<Values...>,
                              type_list_t<std::decay_t<Producers>...>>(
      std::forward<Producers>(producers)...);
  }
};
    
/*
 * Function-like object for building an output list.  It takes the
 * value types as template arguments; the actual producer types are
 * determined from the run-time arguments it is invoked with.
 */
template<typename... Values>
auto constexpr make_output_list = make_output_list_t<Values...>();

} // cuti

#endif
