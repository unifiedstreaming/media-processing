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

#ifndef CUTI_INPUT_LIST_HPP_
#define CUTI_INPUT_LIST_HPP_

#include "consumer.hpp"
#include "linkage.h"
#include "streaming_tag.hpp"
#include "type_list.hpp"

#include <optional>
#include <type_traits>
#include <utility>

namespace cuti
{

/*
 * Interface type consuming of a single input of type Value.
 */
template<typename Value>
struct input_t
{
  input_t()
  { }

  input_t(input_t const&) = delete;
  input_t& operator=(input_t const&) = delete;

  virtual void put(Value value) = 0;

  virtual ~input_t()
  { }
};

/*
 * Interface type consuming a stream of inputs of type Value.
 */
template<typename Value>
struct input_t<streaming_tag_t<Value>> : consumer_t<Value>
{ };

/*
 * Template for implementing input_t<Value>.
 */
template<typename Value, typename Sink>
struct input_impl_t;

/*
 * If the sink type is the same as the value type, we simply take a
 * reference to the value type and assign to it.
 */
template<typename Value>
struct input_impl_t<Value, Value> : input_t<Value>
{
  explicit input_impl_t(Value& target)
  : target_(target)
  { }

  void put(Value value) override
  {
    target_ = std::move(value);
  }

private :
  Value& target_;
};

/*
 * Otherwise, we assume the sink is a callable taking a Value and
 * invoke it.
 */
template<typename Value, typename Sink>
struct input_impl_t : input_t<Value>
{
  template<typename SSink>
  explicit input_impl_t(SSink&& sink)
  : sink_(std::forward<SSink>(sink))
  { }

  void put(Value value) override
  {
    sink_(std::move(value));
  }

private :
  Sink sink_;
};

/*
 * Template for implementing input_t<streaming_tag_t<Value>>.
 *
 * Here, we always assume the sink is a callable taking an optional
 * value and invoke it.
 */
template<typename Value, typename Sink>
struct input_impl_t<streaming_tag_t<Value>, Sink>
: input_t<streaming_tag_t<Value>>
{
  template<typename SSink>
  explicit input_impl_t(SSink&& sink)
  : sink_(std::forward<SSink>(sink))
  { }

  void put(std::optional<Value> value) override
  {
    sink_(std::move(value));
  }

private :
  Sink sink_;
};

/*
 * Abstract base interface for a list of inputs.  Each input has its
 * own value type.
 */
template<typename... Values>
struct input_list_t;

/*
 * Nothing to see here if there are no inputs.  Keep calm and carry
 * on.
 */
template<>
struct CUTI_ABI input_list_t<>
{
  input_list_t()
  { }

  input_list_t(input_list_t const&) = delete;
  input_list_t& operator=(input_list_t const&) = delete;

  virtual ~input_list_t()
  { }
};

/*
 * Otherwise, we require an accessor to get at the first input, and
 * another accessor to get at the other inputs.
 */
template<typename FirstValue, typename... OtherValues>
struct input_list_t<FirstValue, OtherValues...>
{
  input_list_t()
  { }

  input_list_t(input_list_t const&) = delete;
  input_list_t& operator=(input_list_t const&) = delete;

  virtual input_t<FirstValue>& first() = 0;
  virtual input_list_t<OtherValues...>& others() = 0;
  
  virtual ~input_list_t()
  { }
};

/*
 * Template for implementing an input_list_t for the value types in a
 * type list.  Each value type has a corresponding sink type.
 */
template<typename Values, typename Sinks>
struct input_list_impl_t;

/*
 * If there are no value/sink types, we have nothing to do to
 * implement our base interface.
 */
template<>
struct CUTI_ABI input_list_impl_t<type_list_t<>, type_list_t<>>
: input_list_t<>
{ };

/*
 * Otherwise, we implement our base interface by instantiating the
 * first input by its value/sink type, and use recursive instantiation
 * for the other value/sink types.
 */
template<typename FirstValue, typename... OtherValues,
         typename FirstSink, typename... OtherSinks>
struct input_list_impl_t<type_list_t<FirstValue, OtherValues...>,
                         type_list_t<FirstSink, OtherSinks...>>
: input_list_t<FirstValue, OtherValues...>
{
  template<typename FFirstSink, typename... OOtherSinks>
  input_list_impl_t(FFirstSink&& first_sink,
                    OOtherSinks&&... other_sinks)
  : first_(std::forward<FFirstSink>(first_sink))
  , others_(std::forward<OOtherSinks>(other_sinks)...)
  { }

  input_t<FirstValue>& first() override
  {
    return first_;
  }

  input_list_t<OtherValues...>& others() override
  {
    return others_;
  }

private :
  input_impl_t<FirstValue, FirstSink> first_;
  input_list_impl_t<type_list_t<OtherValues...>,
                    type_list_t<OtherSinks...>> others_;
};

/*
 * Helper functor template for building an input list.
 */
template<typename... Values>
struct make_input_list_t
{
  template<typename... Sinks>
  auto operator()(Sinks&&... sinks) const
  {
    return input_list_impl_t<type_list_t<Values...>,
                             type_list_t<std::decay_t<Sinks>...>>(
      std::forward<Sinks>(sinks)...);
  }
};
    
/*
 * Function-like object for building an input list.  It takes the
 * value types as template arguments; the actual sink types are
 * determined from the run-time arguments it is invoked with.
 */
template<typename... Values>
auto constexpr make_input_list = make_input_list_t<Values...>();

} // cuti

#endif
