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

#ifndef CUTI_BASIC_CALLBACK_HPP_
#define CUTI_BASIC_CALLBACK_HPP_

#include "linkage.h"

#include <cassert>

namespace cuti
{

/*
 * Basic minimal callback type avoiding at least some of the usual
 * abstraction penalties.
 */
struct CUTI_ABI basic_callback_t
{
  basic_callback_t()
  : function_(nullptr)
  , arg_(nullptr)
  { }

  basic_callback_t(void (*function)(void*), void* arg)
  : function_((assert(function != nullptr), function))
  , arg_(arg)
  { }

  bool empty() const
  {
    return function_ == nullptr;
  }

  void operator()() const
  {
    assert(!empty());
    (*function_)(arg_);
  }

private :
  void (*function_)(void*);
  void* arg_;
};

} // cuti

#endif
