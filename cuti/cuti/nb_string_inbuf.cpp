/*
 * Copyright (C) 2021-2023 CodeShop B.V.
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

#include "nb_string_inbuf.hpp"

#include "nb_source.hpp"
#include "scheduler.hpp"

#include <ostream>

namespace cuti
{

namespace // anonymous
{

struct nb_string_source_t : nb_source_t
{
  explicit nb_string_source_t(std::string input)
  : input_(std::move(input))
  , pos_(input_.begin())
  { }

  int read(char* first, char const* last, char*& next) override
  {
    std::size_t count = last - first;
    std::size_t available = input_.end() - pos_;
    if(count > available)
    {
      count = available;
    }

    std::copy(pos_, pos_ + count, first);
    pos_ += count;

    next = first + count;
    return 0;
  }

  cancellation_ticket_t call_when_readable(
    scheduler_t& scheduler, callback_t callback) override
  {
    return scheduler.call_alarm(duration_t::zero(), std::move(callback));
  }

  void print(std::ostream& os) const override
  {
    os << "string source@" << this;
  }

private :
  std::string const input_;
  std::string::const_iterator pos_;
};

} // anonymous

std::unique_ptr<nb_inbuf_t>
make_nb_string_inbuf(std::string input, std::size_t bufsize)
{
  return std::make_unique<nb_inbuf_t>(
    std::make_unique<nb_string_source_t>(std::move(input)),
    bufsize);
}

} // cuti
