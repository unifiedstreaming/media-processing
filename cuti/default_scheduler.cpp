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

#include "default_scheduler.hpp"
#include "selector.hpp"

#include <cassert>
#include <thread>
#include <utility>

namespace cuti
{

default_scheduler_t::default_scheduler_t(std::unique_ptr<selector_t> selector)
: alarms_()
, selector_((assert(selector != nullptr), std::move(selector)))
{ }

callback_t default_scheduler_t::wait()
{
  callback_t result = nullptr;

  if(!alarms_.empty())
  {
    int alarm_id = alarms_.front_element();
    auto limit = alarms_.priority(alarm_id);
    do
    {
      auto now = cuti_clock_t::now();
      if(now >= limit)
      {
        result = std::move(alarms_.value(alarm_id));
	assert(result != nullptr);
	alarms_.remove_element(alarm_id);
      }
      else if(selector_->has_work())
      {
        result = selector_->select(limit - now);
      }
      else
      {
        std::this_thread::sleep_for(limit - now);
      }
    } while(result == nullptr);
  }
  else if(selector_->has_work())
  {
    do
    {
      result = selector_->select(duration_t(-1));
    } while(result == nullptr);
  }

  return result;
}

int default_scheduler_t::do_call_alarm(
  time_point_t time_point, callback_t callback)
{
  return alarms_.add_element(time_point, std::move(callback));
}

void default_scheduler_t::do_cancel_alarm(int ticket) noexcept
{
  alarms_.remove_element(ticket);
}

int default_scheduler_t::do_call_when_writable(int fd, callback_t callback)
{
  return selector_->call_when_writable(fd, std::move(callback));
}

void default_scheduler_t::do_cancel_when_writable(int ticket) noexcept
{
  return selector_->cancel_when_writable(ticket);
}

int default_scheduler_t::do_call_when_readable(int fd, callback_t callback)
{
  return selector_->call_when_readable(fd, std::move(callback));
}

void default_scheduler_t::do_cancel_when_readable(int ticket) noexcept
{
  return selector_->cancel_when_readable(ticket);
}

} // cuti

