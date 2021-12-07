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

#include <cuti/async_readers.hpp>
#include <cuti/async_writers.hpp>
#include <cuti/method_map.hpp>

#include <iterator>

#undef NDEBUG
#include <cassert>

namespace // anonymous
{

using namespace cuti;

struct add_handler_t
{
  add_handler_t(logging_context_t& context,
                result_t<void>& result,
                bound_inbuf_t& inbuf,
                bound_outbuf_t& outbuf)
  : context_(context)
  , result_(result)
  , int_reader_(*this, result, inbuf)
  , int_writer_(*this, result, outbuf)
  , first_arg_()
  { }

  void start()
  {
    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "add_handler: " << __func__;
    }

    int_reader_.start(&add_handler_t::on_first_arg);
  }

private :
  void on_first_arg(int arg)
  {
    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "add_handler: " << __func__ << ": arg: " << arg;
    }

    first_arg_ = arg;
    int_reader_.start(&add_handler_t::on_second_arg);
  }

  void on_second_arg(int arg)
  {
    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "add_handler: " << __func__ << ": arg: " << arg;
    }

    int_writer_.start(&add_handler_t::on_done, first_arg_ + arg);
  }

  void on_done()
  {
    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "add_handler: " << __func__;
    }

    result_.submit();
  }
    
private :
  logging_context_t& context_;
  result_t<void>& result_;
  subroutine_t<add_handler_t, reader_t<int>> int_reader_;
  subroutine_t<add_handler_t, writer_t<int>> int_writer_;

  int first_arg_;
};
  
struct subtract_handler_t
{
  subtract_handler_t(logging_context_t& context,
                     result_t<void>& result,
                     bound_inbuf_t& inbuf,
                     bound_outbuf_t& outbuf)
  : context_(context)
  , result_(result)
  , int_reader_(*this, result, inbuf)
  , int_writer_(*this, result, outbuf)
  , first_arg_()
  { }

  void start()
  {
    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "subtract_handler: " << __func__;
    }

    int_reader_.start(&subtract_handler_t::on_first_arg);
  }

private :
  void on_first_arg(int arg)
  {
    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "subtract_handler: " << __func__ << ": arg: " << arg;
    }

    first_arg_ = arg;
    int_reader_.start(&subtract_handler_t::on_second_arg);
  }

  void on_second_arg(int arg)
  {
    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "subtract_handler: " << __func__ << ": arg: " << arg;
    }

    int_writer_.start(&subtract_handler_t::on_done, first_arg_ - arg);
  }

  void on_done()
  {
    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "subtract_handler: " << __func__;
    }

    result_.submit();
  }
    
private :
  logging_context_t& context_;
  result_t<void>& result_;
  subroutine_t<subtract_handler_t, reader_t<int>> int_reader_;
  subroutine_t<subtract_handler_t, writer_t<int>> int_writer_;

  int first_arg_;
};

method_map_t::entry_t const entries[] = {
  { "add", make_method_handler<add_handler_t> },
  { "subtract", make_method_handler<subtract_handler_t> }
};

method_map_t const method_map{std::begin(entries), std::end(entries)};
  
void test_method_map()
{
  assert(method_map.find_entry(identifier_t("a")) == nullptr);
  assert(method_map.find_entry(identifier_t("add")) == &entries[0]);
  assert(method_map.find_entry(identifier_t("divide")) == nullptr);
  assert(method_map.find_entry(identifier_t("multiply")) == nullptr);
  assert(method_map.find_entry(identifier_t("subtract")) == &entries[1]);
  assert(method_map.find_entry(identifier_t("z")) == nullptr);
}
  
} // anonymous

int main(int argc, char* argv[])
{
  test_method_map();

  return 0;
}
