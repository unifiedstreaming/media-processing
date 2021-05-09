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
#include <utility>

namespace cuti
{

default_scheduler_t::default_scheduler_t(std::unique_ptr<selector_t> selector)
: selector_((assert(selector != nullptr), std::move(selector)))
{ }

bool default_scheduler_t::has_work() const noexcept
{
  return selector_->has_work();
}

callback_t default_scheduler_t::poll()
{
  assert(this->has_work());
  return selector_->select(selector_t::timeout_t::zero());
}

callback_t default_scheduler_t::wait()
{
  assert(this->has_work());

  callback_t result;
  do
  {
    result = selector_->select(selector_t::timeout_t(-1));
  } while(result == nullptr);
  return result;
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

default_scheduler_t::~default_scheduler_t()
{ }

} // cuti

