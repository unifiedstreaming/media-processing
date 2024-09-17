/*
 * Copyright (C) 2021-2024 CodeShop B.V.
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

#include "echo_handler.hpp"

#include "quoted.hpp"

#include <stdexcept>
#include <utility>

namespace cuti
{

echo_handler_t::echo_handler_t(result_t<void>& result,
                               logging_context_t const& context,
                               bound_inbuf_t& inbuf,
                               bound_outbuf_t& outbuf,
                               std::optional<std::string> censored)
: result_(result)
, censored_(std::move(censored))
, begin_reader_(*this, result_, inbuf)
, begin_writer_(*this, result_, outbuf)
, end_checker_(*this, result_, inbuf)
, end_writer_(*this, result_, outbuf)
, element_reader_(*this, result_, inbuf)
, element_writer_(*this, result_, outbuf)
{ }

void echo_handler_t::start(stack_marker_t& base_marker)
{
  begin_reader_.start(base_marker, &echo_handler_t::write_begin);
}

void echo_handler_t::write_begin(stack_marker_t& base_marker)
{
  begin_writer_.start(base_marker, &echo_handler_t::echo_elements);
}

void echo_handler_t::echo_elements(stack_marker_t& base_marker)
{
  end_checker_.start(base_marker, &echo_handler_t::on_end_checker);
}

void echo_handler_t::on_end_checker(stack_marker_t& base_marker, bool at_end)
{
  if(at_end)
  {
    end_writer_.start(base_marker, &echo_handler_t::on_end_written);
    return;
  }

  element_reader_.start(base_marker, &echo_handler_t::write_element);
}

void echo_handler_t::on_end_written(stack_marker_t& base_marker)
{
  result_.submit(base_marker);
}

void echo_handler_t::write_element(
  stack_marker_t& base_marker, std::string value)
{
  if(censored_ && value == *censored_)
  {
    result_.fail(base_marker, std::make_exception_ptr(
      std::runtime_error(value + " is censored")));
    return;
  }

  element_writer_.start(
    base_marker, &echo_handler_t::echo_elements, std::move(value));
}

} // cuti
