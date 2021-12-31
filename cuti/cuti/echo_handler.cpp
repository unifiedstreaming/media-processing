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

#include "echo_handler.hpp"

#include "quoted_string.hpp"

#include <stdexcept>
#include <utility>

namespace cuti
{

echo_handler_t::echo_handler_t(result_t<void>& result,
                               logging_context_t& context,
                               bound_inbuf_t& inbuf,
                               bound_outbuf_t& outbuf,
                               std::optional<std::string> censored)
: result_(result)
, context_(context)
, censored_(std::move(censored))
, begin_reader_(*this, result_, inbuf)
, begin_writer_(*this, result_, outbuf)
, end_checker_(*this, result_, inbuf)
, end_writer_(*this, result_, outbuf)
, element_reader_(*this, result_, inbuf)
, element_writer_(*this, result_, outbuf)
{ }

void echo_handler_t::start()
{
  if(auto msg = context_.message_at(loglevel_t::info))
  {
    *msg << "echo_handler: " << __func__;
  }

  begin_reader_.start(&echo_handler_t::write_begin);
}

void echo_handler_t::write_begin()
{
  if(auto msg = context_.message_at(loglevel_t::info))
  {
    *msg << "echo_handler: " << __func__;
  }

  begin_writer_.start(&echo_handler_t::echo_elements);
}

void echo_handler_t::echo_elements()
{
  if(auto msg = context_.message_at(loglevel_t::info))
  {
    *msg << "echo_handler: " << __func__;
  }

  end_checker_.start(&echo_handler_t::on_end_checker);
}

void echo_handler_t::on_end_checker(bool at_end)
{
  if(auto msg = context_.message_at(loglevel_t::info))
  {
    *msg << "echo_handler: " << __func__ << " (at_end: " << at_end << ')';
  }

  if(at_end)
  {
    end_writer_.start(&echo_handler_t::on_end_written);
    return;
  }

  element_reader_.start(&echo_handler_t::write_element);
}

void echo_handler_t::on_end_written()
{
  if(auto msg = context_.message_at(loglevel_t::info))
  {
    *msg << "echo_handler: " << __func__;
  }

  result_.submit();
}

void echo_handler_t::write_element(std::string value)
{
  if(auto msg = context_.message_at(loglevel_t::info))
  {
    *msg << "echo_handler: " << __func__ << "(value: " <<
      quoted_string(value) << ')';
  }

  if(censored_ && value == *censored_)
  {
    result_.fail(std::runtime_error(value + " is censored"));
    return;
  }

  element_writer_.start(&echo_handler_t::echo_elements, std::move(value));
}

} // cuti
