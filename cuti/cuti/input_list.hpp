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

#include <optional>
#include <type_traits>
#include <utility>

#include "linkage.h"
#include "streaming_tag.hpp"
#include "type_list.hpp"

namespace cuti
{

/*
 * Abstract base interface for handling an input of type Value.
 */
template<typename Value>
struct input_t
{
  input_t()
  { }

  input_t(input_t const&) = delete;
  input_t& operator=(input_t const&) = delete;

  virtual void set(Value value) const = 0;

  virtual ~input_t()
  { }
};

/*
 * Abstract base interface for handling a stream of inputs of type
 * Value.  The end of the stream is marked by an empty optional.
 */
template<typename Value>
struct input_t<streaming_tag_t<Value>>
{
  input_t()
  { }

  input_t(input_t const&) = delete;
  input_t& operator=(input_t const&) = delete;

  virtual void set(std::optional<Value> value) const = 0;

  virtual ~input_t()
  { }
};

/*
 * Template for implementing input_t<Value>.
 */
template<typename Value, typename Handler>
struct input_impl_t;

/*
 * If the handler type matches the value type, we simply take a
 * reference to the value type and assign to it.
 */
template<typename Value>
struct input_impl_t<Value, Value> : input_t<Value>
{
  explicit input_impl_t(Value& target)
  : target_(target)
  { }

  void set(Value value) const override
  {
    target_ = std::move(value);
  }

private :
  Value& target_;
};

/*
 * Otherwise, if the value type is taggged as streaming, we assume
 * the handler is a const-callable taking an optional Value and
 * invoke it.
 */
template<typename Value, typename Handler>
struct input_impl_t<streaming_tag_t<Value>, Handler>
: input_t<streaming_tag_t<Value>>
{
  template<typename HHandler>
  explicit input_impl_t(HHandler&& handler)
  : handler_(std::forward<HHandler>(handler))
  { }

  void set(std::optional<Value> value) const override
  {
    handler_(std::move(value));
  }

private :
  Handler handler_;
};

/*
 * Otherwise, we assume the handler is a const-callable taking a Value
 * and invoke it.
 */
template<typename Value, typename Handler>
struct input_impl_t : input_t<Value>
{
  template<typename HHandler>
  explicit input_impl_t(HHandler&& handler)
  : handler_(std::forward<HHandler>(handler))
  { }

  void set(Value value) const override
  {
    handler_(std::move(value));
  }

private :
  Handler handler_;
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

  virtual input_t<FirstValue> const& first() const = 0;
  virtual input_list_t<OtherValues...> const& others() const = 0;
  
  virtual ~input_list_t()
  { }
};

/*
 * Template for implementing an input_list_t for the value types in a
 * type list.  Each value type has a corresponding Handler type.
 */
template<typename Values, typename Handlers>
struct input_list_impl_t;

/*
 * If there are no value/handler types, we have nothing to add to our
 * base interface.
 */
template<>
struct CUTI_ABI input_list_impl_t<type_list_t<>, type_list_t<>>
: input_list_t<>
{ };

/*
 * Otherwise, we implement our base interface by instantiating the
 * first input by its value/handler type, and use recursive
 * instantiation for the other value/handler types.
 */
template<typename FirstValue, typename... OtherValues,
         typename FirstHandler, typename... OtherHandlers>
struct input_list_impl_t<type_list_t<FirstValue, OtherValues...>,
                         type_list_t<FirstHandler, OtherHandlers...>>
: input_list_t<FirstValue, OtherValues...>
{
  template<typename FFirstHandler, typename... OOtherHandlers>
  input_list_impl_t(FFirstHandler&& first_handler,
                    OOtherHandlers&&... other_handlers)
  : first_(std::forward<FFirstHandler>(first_handler))
  , others_(std::forward<OOtherHandlers>(other_handlers)...)
  { }

  input_t<FirstValue> const& first() const override
  {
    return first_;
  }

  input_list_t<OtherValues...> const& others() const override
  {
    return others_;
  }

private :
  input_impl_t<FirstValue, FirstHandler> first_;
  input_list_impl_t<type_list_t<OtherValues...>,
                    type_list_t<OtherHandlers...>> others_;
};

/*
 * Helper functor template for building an input list.
 */
template<typename... Values>
struct make_input_list_t
{
  template<typename... Handlers>
  auto operator()(Handlers&&... handlers) const
  {
    return input_list_impl_t<type_list_t<Values...>,
                             type_list_t<std::decay_t<Handlers>...>>(
      std::forward<Handlers>(handlers)...);
  }
};
    
/*
 * Function-like object for building an input list.  It takes the
 * value types as template arguments; the actual handler types are
 * determined from the run-time parameters it is invoked with.
 */
template<typename... Values>
auto constexpr make_input_list = make_input_list_t<Values...>();

} // cuti

#endif
