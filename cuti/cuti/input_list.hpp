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

#include <type_traits>
#include <utility>

#include "linkage.h"
#include "type_list.hpp"

namespace cuti
{

/*
 * Abstract base interface for handling an input value of type T.
 */
template<typename T>
struct input_t
{
  input_t()
  { }

  input_t(input_t const&) = delete;
  input_t& operator=(input_t const&) = delete;

  virtual void set(T value) const = 0;

  virtual ~input_t()
  { }
};

/*
 * Template for implementing input_t<T>.
 */
template<typename T, typename Handler>
struct input_impl_t;

/*
 * If the handler type matches the value type, we simply take a
 * reference to the value type and assign to it...
 */
template<typename T>
struct input_impl_t<T, T> : input_t<T>
{
  explicit input_impl_t(T& target)
  : target_(target)
  { }

  void set(T value) const override
  {
    target_ = std::move(value);
  }

private :
  T& target_;
};

/*
 * ...otherwise, we assume the handler type is a (const-)callable
 * taking a T and invoke it.
 */
template<typename T, typename Handler>
struct input_impl_t : input_t<T>
{
  template<typename HHandler>
  explicit input_impl_t(HHandler&& handler)
  : handler_(std::forward<HHandler>(handler))
  { }

  void set(T value) const override
  {
    handler_(std::move(value));
  }

private :
  Handler handler_;
};

/*
 * Abstract base interface for a list of inputs.  Each input handles a
 * specific value type.
 */
template<typename... Types>
struct input_list_t;

/*
 * Nothing to see here if there are no input types.  Keep calm and
 * carry on.
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
 * Otherwise, the list of input types is non-empty.  We require an
 * accessor to get at the first input handler, and another accessor to
 * get at the other input handlers.
 */
template<typename FirstType, typename... OtherTypes>
struct input_list_t<FirstType, OtherTypes...>
{
  input_list_t()
  { }

  input_list_t(input_list_t const&) = delete;
  input_list_t& operator=(input_list_t const&) = delete;

  virtual input_t<FirstType> const& first() const = 0;
  virtual input_list_t<OtherTypes...> const& others() const = 0;
  
  virtual ~input_list_t()
  { }
};

/*
 * Template for implementing an input_list_t for the types in a type
 * list.  Each type takes its own Handler implementation.
 */
template<typename TypeList, typename... Handlers>
struct input_list_impl_t;

/*
 * If the list of types is empty, there are no handlers and we have
 * nothing to add to our base interface.
 */
template<>
struct CUTI_ABI input_list_impl_t<type_list_t<>> : input_list_t<>
{ };

/*
 * Otherwise, we implement our base interface by instantiating the
 * first input by its type and its handler, and use recursive
 * instantiation for the other input and handler types.
 */
template<typename FirstType, typename... OtherTypes,
         typename FirstHandler, typename... OtherHandlers>
struct input_list_impl_t<
  type_list_t<FirstType, OtherTypes...>, FirstHandler, OtherHandlers...>
: input_list_t<FirstType, OtherTypes...>
{
  template<typename FFirstHandler, typename... OOtherHandlers>
  input_list_impl_t(FFirstHandler&& first_handler,
                    OOtherHandlers&&... other_handlers)
  : first_(std::forward<FFirstHandler>(first_handler))
  , others_(std::forward<OOtherHandlers>(other_handlers)...)
  { }

  input_t<FirstType> const& first() const override
  {
    return first_;
  }

  input_list_t<OtherTypes...> const& others() const override
  {
    return others_;
  }

private :
  input_impl_t<FirstType, FirstHandler> first_;
  input_list_impl_t<type_list_t<OtherTypes...>, OtherHandlers...> others_;
};

/*
 * Helper functor template for building an input list.
 */
template<typename... Types>
struct make_input_list_t
{
  template<typename... Handlers>
  auto operator()(Handlers&&... handlers) const
  {
    return input_list_impl_t<
      type_list_t<Types...>, std::decay_t<Handlers>...>(
        std::forward<Handlers>(handlers)...);
  }
};
    
/*
 * Function-like object for building an input list.  It takes the
 * input types as template arguments; the actual handlers are taken
 * from the run-time parameters it is invoked with.
 */
template<typename... Types>
auto constexpr make_input_list = make_input_list_t<Types...>();

} // cuti

#endif
