/*
 * Copyright (C) 2021-2026 CodeShop B.V.
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
#include "producer.hpp"
#include "sequence.hpp"
#include "type_list.hpp"

#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

namespace cuti
{

/*
 * Interface type producing of a single output of type Value.
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
 * Interface type producing a stream of outputs of type Value.
 */
template<typename Value>
struct output_t<sequence_t<Value>> : producer_t<Value>
{ };

/*
 * Template for implementing output_t<Value>
 */
template<typename Value, typename Source>
struct output_impl_t : output_t<Value>
{
  template<typename SSource>
  explicit output_impl_t(SSource&& source)
  : source_(std::forward<SSource>(source))
  { }

  Value get() override
  {
    if constexpr(std::is_invocable_r_v<Value, Source>)
    {
      // assume a callable was captured
      return source_();
    }
    else
    {
      // assume a value was captured
      return std::move(source_);
    }
  }

private :
  Source source_;
};

/*
 * Template for implementing output_t<sequence_t<Value>>.
 *
 * If the Source is a std::vector<Value>, we capture its value and
 * produce its elements one at a time.
 */
template<typename Value>
struct output_impl_t<sequence_t<Value>, std::vector<Value>>
: output_t<sequence_t<Value>>
{
  template<typename SSource>
  explicit output_impl_t(SSource&& source)
  : source_(std::forward<SSource>(source))
  , first_(source_.begin())
  , last_(source_.end())
  { }

  std::optional<Value> get() override
  {
    std::optional<Value> result = std::nullopt;

    auto pos = first_;
    if(pos != last_)
    {
      ++first_;
      result.emplace(std::move(*pos));
    }

    return result;
  }

private :
  std::vector<Value> source_;
  typename std::vector<Value>::iterator first_;
  typename std::vector<Value>::iterator last_;
};
    
/*
 * Otherwise, we assume that we captured a callable and simply
 * invoke it.
 */
template<typename Value, typename Source>
struct output_impl_t<sequence_t<Value>, Source>
: output_t<sequence_t<Value>>
{
  template<typename SSource>
  explicit output_impl_t(SSource&& source)
  : source_(std::forward<SSource>(source))
  { }

  std::optional<Value> get() override
  {
    // always assume a callable was captured
    return source_();
  }

private :
  Source source_;
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
 * type list.  Each value type has a corresponding source type.
 */
template<typename Values, typename Sources>
struct output_list_impl_t;

/*
 * If there are no value/source types, we have nothing to do to
 * implement our base interface.
 */
template<>
struct CUTI_ABI output_list_impl_t<type_list_t<>, type_list_t<>>
: output_list_t<>
{ };

/*
 * Otherwise, we implement our base interface by instantiating the
 * first output by its value/source type, and use recursive
 * instantiation for the other value/source types.
 */
template<typename FirstValue, typename... OtherValues,
         typename FirstSource, typename... OtherSources>
struct output_list_impl_t<type_list_t<FirstValue, OtherValues...>,
                          type_list_t<FirstSource, OtherSources...>>
: output_list_t<FirstValue, OtherValues...>
{
  template<typename FFirstSource, typename... OOtherSources>
  output_list_impl_t(FFirstSource&& first_source,
                     OOtherSources&&... other_sources)
  : first_(std::forward<FFirstSource>(first_source))
  , others_(std::forward<OOtherSources>(other_sources)...)
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
  output_impl_t<FirstValue, FirstSource> first_;
  output_list_impl_t<type_list_t<OtherValues...>,
                     type_list_t<OtherSources...>> others_;
};

/*
 * Helper functor template for building an output list.
 */
template<typename... Values>
struct make_output_list_t
{
  template<typename... Sources>
  auto operator()(Sources&&... sources) const
  {
    using impl_t = output_list_impl_t<
      type_list_t<Values...>,
      type_list_t<std::decay_t<Sources>...>
    >;
    return impl_t(std::forward<Sources>(sources)...);
  }
};
    
/*
 * Specialization for building an output list from a type list.
 */
template<typename... Values>
struct make_output_list_t<type_list_t<Values...>>
{
  template<typename... Sources>
  auto operator()(Sources&&... sources) const
  {
    using impl_t = output_list_impl_t<
      type_list_t<Values...>,
      type_list_t<std::decay_t<Sources>...>
    >;
    return impl_t(std::forward<Sources>(sources)...);
  }
};
    
/*
 * Function-like object for building an output list.  It takes the
 * value types (or a type list) as template arguments; the actual
 * source types are determined from the run-time arguments it is
 * invoked with.
 */
template<typename... Values>
constexpr auto make_output_list = make_output_list_t<Values...>();

/*
 * Helper functor template for building an abstract output list.
 */
template<typename... Values>
struct make_output_list_ptr_t
{
  template<typename... Sources>
  std::unique_ptr<output_list_t<Values...>>
  operator()(Sources&&... sources) const
  {
    using impl_t = output_list_impl_t<
      type_list_t<Values...>,
      type_list_t<std::decay_t<Sources>...>
    >;
    return std::make_unique<impl_t>(std::forward<Sources>(sources)...);
  }
};
    
/*
 * Specialization for building an abstract output list from a type list.
 */
template<typename... Values>
struct make_output_list_ptr_t<type_list_t<Values...>>
{
  template<typename... Sources>
  std::unique_ptr<output_list_t<Values...>>
  operator()(Sources&&... sources) const
  {
    using impl_t = output_list_impl_t<
      type_list_t<Values...>,
      type_list_t<std::decay_t<Sources>...>
    >;
    return std::make_unique<impl_t>(std::forward<Sources>(sources)...);
  }
};
    
/*
 * Function-like object for building an abstract output list.  It
 * takes the value types (or a type list) as template arguments; the
 * actual source types are determined from the run-time arguments it
 * is invoked with.
 */
template<typename... Values>
constexpr auto make_output_list_ptr = make_output_list_ptr_t<Values...>();

} // cuti

#endif
