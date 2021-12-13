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

#ifndef CUTI_METHOD_HPP_
#define CUTI_METHOD_HPP_

#include "subroutine.hpp"

#include <exception>
#include <memory>
#include <utility>

namespace cuti
{

/*
 * Interface type for an asynchronous method instance living under
 * request handler type Parent.
 */
template<typename Parent>
struct method_t
{
  method_t()
  { }

  method_t(method_t const&) = delete;
  method_t& operator=(method_t const&) = delete;

  virtual void start(void (Parent::*on_success)()) = 0;

  virtual ~method_t()
  { }
};

/*
 * Concrete asynchronous method instance type delegating to
 * asynchronous routine type Impl.
 */
template<typename Parent, typename Impl>
struct method_inst_t : method_t<Parent>
{
  template<typename... ImplArgs>
  method_inst_t(Parent& parent,
                void (Parent::*on_failure)(std::exception_ptr),
                ImplArgs&&... impl_args)
  : subroutine_(parent, on_failure, std::forward<ImplArgs>(impl_args)...)
  { }

  void start(void (Parent::*on_success)()) override
  {
    subroutine_.start(on_success);
  }

private :
  subroutine_t<Parent, Impl, failure_mode_t::handle_in_parent> subroutine_;
};

/*
 * Convenience function for creating a method instance.
 */
template<typename Impl, typename Parent, typename... ImplArgs>
std::unique_ptr<method_t<Parent>>
make_method(Parent& parent,
            void (Parent::*on_failure)(std::exception_ptr),
            ImplArgs&&... impl_args)
{
  return std::make_unique<method_inst_t<Parent, Impl>>(
    parent, on_failure, std::forward<ImplArgs>(impl_args)...);
}

} // cuti

#endif
