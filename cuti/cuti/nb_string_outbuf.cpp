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

#include "nb_string_outbuf.hpp"

#include "nb_sink.hpp"
#include "scheduler.hpp"

#include <ostream>

namespace cuti
{

namespace // anonymous
{

struct nb_string_sink_t : nb_sink_t
{
  explicit nb_string_sink_t(std::string& output)
  : output_(output)
  { }

  int write(char const* first, char const* last, char const*& next) override
  {
    output_.insert(output_.end(), first, last);

    next = last;
    return 0;
  }

  cancellation_ticket_t call_when_writable(
    scheduler_t& scheduler, callback_t callback) override
  {
    return scheduler.call_alarm(duration_t::zero(), std::move(callback));
  }

  void print(std::ostream& os) const override
  {
    os << "string sink@" << this;
  }

private :
  std::string& output_;
};

} // anonymous

std::unique_ptr<nb_outbuf_t>
make_nb_string_outbuf(logging_context_t& context,
                      std::string& output,
		      std::size_t bufsize)
{
  return std::make_unique<nb_outbuf_t>(
    context,
    std::make_unique<nb_string_sink_t>(output),
    bufsize);
}

} // cuti
