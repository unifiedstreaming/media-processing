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

#ifndef CUTI_FINAL_RESULT_HPP_
#define CUTI_FINAL_RESULT_HPP_

#include "result.hpp"

#include <cassert>
#include <exception>
#include <variant>

namespace cuti
{

template<typename T>
struct final_result_t : result_t<T>
{
  bool available() const
  {
    return state_.index() != 0;
  }

  const T& value() const
  {
    assert(this->available());

    if(auto ex = this->exception())
    {
      std::rethrow_exception(std::move(ex));
    }

    return std::get<1>(state_);
  }

  std::exception_ptr exception() const
  {
    assert(this->available());

    return state_.index() == 2 ? std::get<2>(state_) : std::exception_ptr();
  }
    
private :
  void do_submit(T value) override
  {
    assert(!this->available());

    state_.template emplace<1>(std::move(value));
  }

  void do_fail(std::exception_ptr ex) override
  {
    assert(!this->available());

    state_.template emplace<2>(std::move(ex));
  }    

private :
  std::variant<std::monostate, T, std::exception_ptr> state_;
};

} // cuti

#endif
