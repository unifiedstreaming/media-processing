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

#include "add_handler.hpp"

namespace cuti
{

add_handler_t::add_handler_t(logging_context_t& context,
                             result_t<void>& result,
                             bound_inbuf_t& inbuf,
                             bound_outbuf_t& outbuf)
: context_(context)
, result_(result)
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
