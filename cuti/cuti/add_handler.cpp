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

#include "add_handler.hpp"

#include <limits>

namespace cuti
{

add_handler_t::add_handler_t(result_t<void>& result,
                             logging_context_t const& context,
                             bound_inbuf_t& inbuf,
                             bound_outbuf_t& outbuf)
: result_(result)
, context_(context)
, int_reader_(*this, result, inbuf)
, int_writer_(*this, result, outbuf)
, first_arg_()
{ }

void add_handler_t::start()
{
  if(auto msg = context_.message_at(loglevel_t::info))
  {
    *msg << "add_handler: " << __func__;
  }

  int_reader_.start(&add_handler_t::on_first_arg);
}

void add_handler_t::on_first_arg(int arg)
{
  if(auto msg = context_.message_at(loglevel_t::info))
  {
    *msg << "add_handler: " << __func__ << ": arg: " << arg;
  }

  first_arg_ = arg;
  int_reader_.start(&add_handler_t::on_second_arg);
}

void add_handler_t::on_second_arg(int arg)
{
  if(auto msg = context_.message_at(loglevel_t::info))
  {
    *msg << "add_handler: " << __func__ << ": arg: " << arg;
  }

  if(first_arg_ >= 0)
  {
    if(arg > std::numeric_limits<int>::max() - first_arg_)
    {
      result_.fail(std::runtime_error("addition overflow"));
      return;
    }
  }
  else
  {
    if(arg < std::numeric_limits<int>::min() - first_arg_)
    {
      result_.fail(std::runtime_error("addition underflow"));
      return;
    }
  }
    
  int_writer_.start(&add_handler_t::on_done, first_arg_ + arg);
}

void add_handler_t::on_done()
{
  if(auto msg = context_.message_at(loglevel_t::info))
  {
    *msg << "add_handler: " << __func__;
  }

  result_.submit();
}

} // cuti
