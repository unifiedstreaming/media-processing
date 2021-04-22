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

#include "selector.hpp"

#include "logging_context.hpp"

namespace cuti
{

selector_t::~selector_t()
{ }

void run_selector(logging_context_t& context,
                  loglevel_t loglevel,
                  selector_t& selector,
                  selector_t::timeout_t timeout)
{
  assert(timeout >= selector_t::timeout_t::zero());

  auto now = std::chrono::system_clock::now();
  auto const limit = now + timeout;

  do
  {
    if(!selector.has_work())
    {
      break;
    }
    
    timeout = limit - now; 
    if(auto msg = context.message_at(loglevel))
    {
      auto milliseconds = std::chrono::duration_cast<
        std::chrono::milliseconds>(timeout).count();
      *msg << "run_selector(): awaiting callback for " <<
        milliseconds << " millisecond(s)...";
    }
    
    auto callback = selector.select(timeout);
    if(callback == nullptr)
    {
      if(auto msg = context.message_at(loglevel))
      {
        *msg << "run_selector(): got empty callback";
      }
    }
    else
    {
      if(auto msg = context.message_at(loglevel))
      {
        *msg << "run_selector(): invoking callback";
      }
      callback();
    }

    now = std::chrono::system_clock::now();
  } while(now < limit);

  if(auto msg = context.message_at(loglevel))
  {
    if(selector.has_work())
    {
      *msg << "run_selector(): timeout";
    }
    else
    {
      *msg << "run_selector(): out of work";
    }
  }
}

} // cuti
