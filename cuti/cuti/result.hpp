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

#ifndef CUTI_RESULT_HPP_
#define CUTI_RESULT_HPP_

#include "linkage.h"

#include <exception>
#include <utility>

namespace cuti
{

/*
 * Strawman type for reporting no meaningful value.
 */
struct CUTI_ABI no_value_t { };

/*
 * Interface for reporting the result of an asynchronous operation.
 */
template<typename T = no_value_t>
struct result_t
{
  result_t()
  { }

  result_t(result_t const&) = delete;
  result_t& operator=(result_t const&) = delete;
  
  void set_value(T value = no_value_t{})
  {
    this->do_set_value(std::move(value));
  }

  void set_exception(std::exception_ptr ex)
  {
    assert(ex != nullptr);
    this->do_set_exception(std::move(ex));
  }

  template<typename Ex>
  void set_exception(Ex&& ex)
  {
    this->do_set_exception(std::make_exception_ptr(std::forward<Ex>(ex)));
  }
  
  virtual ~result_t()
  { }

private :
  virtual void do_set_value(T value) = 0;
  virtual void do_set_exception(std::exception_ptr ex) = 0;
};

} // cuti

#endif
