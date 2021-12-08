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

#include "subtract_handler.hpp"

namespace cuti
{

subtract_handler_t::subtract_handler_t(result_t<void>& result,
                                       logging_context_t& context,
                                       bound_inbuf_t& inbuf,
                                       bound_outbuf_t& outbuf)
: result_(result)
, context_(context)
, int_reader_(*this, result, inbuf)
, int_writer_(*this, result, outbuf)
, first_arg_()
{ }

void subtract_handler_t::start()
{
  if(auto msg = context_.message_at(loglevel_t::info))
  {
    *msg << "subtract_handler: " << __func__;
  }

  int_reader_.start(&subtract_handler_t::on_first_arg);
}

void subtract_handler_t::on_first_arg(int arg)
{
  if(auto msg = context_.message_at(loglevel_t::info))
  {
    *msg << "subtract_handler: " << __func__ << ": arg: " << arg;
  }

  first_arg_ = arg;
  int_reader_.start(&subtract_handler_t::on_second_arg);
}

void subtract_handler_t::on_second_arg(int arg)
{
  if(auto msg = context_.message_at(loglevel_t::info))
  {
    *msg << "subtract_handler: " << __func__ << ": arg: " << arg;
  }

  if(arg >= 0)
  {
    if(first_arg_ < std::numeric_limits<int>::min() + arg)
    {
      result_.fail(std::runtime_error("subtraction underflow"));
      return;
    }
  }
  else
  {
    if(first_arg_ > std::numeric_limits<int>::max() + arg)
    {
      result_.fail(std::runtime_error("subtraction overflow"));
      return;
    }
  }
    
  int_writer_.start(&subtract_handler_t::on_done, first_arg_ - arg);
}

void subtract_handler_t::on_done()
{
  if(auto msg = context_.message_at(loglevel_t::info))
  {
    *msg << "subtract_handler: " << __func__;
  }

  result_.submit();
}

} // cuti
