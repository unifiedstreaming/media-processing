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

#include <cuti/async_inbuf.hpp>
#include <cuti/async_outbuf.hpp>
#include <cuti/async_tcp_input_adapter.hpp>
#include <cuti/async_tcp_output_adapter.hpp>
#include <cuti/default_scheduler.hpp>
#include <cuti/tcp_connection.hpp>

#include <string>
#include <tuple>

#undef NDEBUG
#include <cassert>

namespace // anoynmous
{

using namespace cuti;

std::size_t constexpr default_bufsize = async_outbuf_t::default_bufsize;

struct writer_t
{
  writer_t(bool bulk,
           scheduler_t& sched, async_outbuf_t& out,
           char const* first, char const* last)
  : bulk_(bulk)
  , sched_(sched)
  , out_(out)
  , first_(first)
  , last_(last)
  , done_(false)
  {
    out_.call_when_writable(sched_, [this] { this->write_chars(); });
  }

  writer_t(writer_t const&) = delete;
  writer_t& operator=(writer_t const&) = delete;

  bool done() const
  {
    return done_;
  }

private :
  void write_chars()
  {
    assert(out_.writable());

    if(bulk_)
    {
      while(out_.writable() && first_ != last_)
      {
        first_ = out_.write(first_, last_);
      }
    }
    else
    {
      while(out_.writable() && first_ != last_)
      {
        out_.put(*first_);
        ++first_;
      }
    }

    if(first_ != last_)
    {
      out_.call_when_writable(sched_, [this] { this->write_chars(); });
    }
    else
    {
      out_.call_when_writable(sched_, [this] { this->start_flush(); });
    }
  }

  void start_flush()
  {
    assert(out_.writable());

    out_.start_flush();
    out_.call_when_writable(sched_, [this] { this->set_done(); });
  }

  void set_done()
  {
    assert(out_.writable());

    done_ = true;
  }

private :
  bool const bulk_;
  scheduler_t& sched_;
  async_outbuf_t& out_;
  char const* first_;
  char const* last_;
  bool done_;
};

struct reader_t
{
  reader_t(bool bulk,
           scheduler_t& sched, async_inbuf_t& in)
  : bulk_(bulk)
  , sched_(sched)
  , in_(in)
  , done_(false)
  , result_()
  {
    in_.call_when_readable(sched_, [this] { this->read_chars(); });
  }

  reader_t(reader_t const&) = delete;
  reader_t& operator=(reader_t const&) = delete;

  bool done() const
  {
    return done_;
  }

  std::string const& result() const
  {
    return result_;
  }

private :
  void read_chars()
  {
    assert(in_.readable());

    if(bulk_)
    {
      char buf[65536];
      char* next;
      while(in_.readable() &&
            (next = in_.read(buf, buf + sizeof buf)) != buf)
      {
        result_.append(buf, next);
      }
    }
    else
    {
      int c;
      while(in_.readable() && (c = in_.peek()) != async_inbuf_t::eof)
      {
        result_ += static_cast<char>(c);
        in_.skip();
      }
    }

    if(!in_.readable())
    {
      in_.call_when_readable(sched_, [this] { this->read_chars(); });
    }
    else
    {
      done_ = true;
    }
  }

private :
  bool bulk_;
  scheduler_t& sched_;
  async_inbuf_t& in_;
  bool done_;
  std::string result_;
};
  
void do_test_echo(bool bulk,
                  std::string const& str,
                  std::size_t outbufsize,
                  std::size_t inbufsize)
{
  default_scheduler_t scheduler;

  std::shared_ptr<tcp_connection_t> conn_out;
  std::shared_ptr<tcp_connection_t> conn_in;
  std::tie(conn_out, conn_in) = make_connected_pair();
  conn_out->set_nonblocking();
  conn_in->set_nonblocking();

  async_outbuf_t outbuf(
    std::make_unique<async_tcp_output_adapter_t>(conn_out),
    outbufsize);
  async_inbuf_t inbuf(
    std::make_unique<async_tcp_input_adapter_t>(conn_in),
    inbufsize);

  char const* first = str.empty() ? nullptr : str.data();
  char const* last = first + str.size();

  writer_t writer(bulk, scheduler, outbuf, first, last);
  reader_t reader(bulk, scheduler, inbuf);

  while(!writer.done())
  {
    callback_t callback = scheduler.wait();
    assert(callback != nullptr);
    callback();
  }

  assert(outbuf.error_status() == 0);
  conn_out->close_write_end();
  
  while(!reader.done())
  {
    callback_t callback = scheduler.wait();
    assert(callback != nullptr);
    callback();
  }
    
  assert(reader.result() == str);
  assert(inbuf.error_status() == 0);
}

std::string make_long_string()
{
  int i = 1;
  std::string result;

  while(result.length() <= 2 * default_bufsize)
  {
    result += "Segment ";
    result += std::to_string(i);
    result += ' ';
    ++i;
  }

  return result;
}

void test_echo()
{
  std::string const empty_string = "";
  std::string const small_string = "Karl Heinz Stockhausen";
  std::string const long_string = make_long_string();

  static size_t constexpr tiny_buf = 1;
  static size_t constexpr small_buf = 10;
  static size_t constexpr large_buf = default_bufsize;

  for(auto bulk : { false, true })
  {
    for(auto const& str : { empty_string, small_string })
    {
      for(auto outbufsize : { tiny_buf, small_buf, large_buf })
      {
        for(auto inbufsize : { tiny_buf, small_buf, large_buf })
        {
          do_test_echo(bulk, str, outbufsize, inbufsize);
        }
      }
    }

    for(auto outbufsize : { small_buf, large_buf })
    {
      for(auto inbufsize : { small_buf, large_buf })
      {
        do_test_echo(bulk, long_string, outbufsize, inbufsize);
      }
    }
  }
}

void do_test_error_status(bool bulk)
{
  default_scheduler_t scheduler;

  std::shared_ptr<tcp_connection_t> conn_out;
  std::shared_ptr<tcp_connection_t> conn_in;
  std::tie(conn_out, conn_in) = make_connected_pair();
  conn_out->set_nonblocking();
  conn_in.reset();

  async_outbuf_t outbuf(
    std::make_unique<async_tcp_output_adapter_t>(conn_out), default_bufsize);

  std::string str = make_long_string();
  char const* first = str.empty() ? nullptr : str.data();
  char const* last = first + str.size();

  writer_t writer(bulk, scheduler, outbuf, first, last);

  while(!writer.done())
  {
    callback_t callback = scheduler.wait();
    assert(callback != nullptr);
    callback();
  }

  assert(outbuf.error_status() != 0);
}

void test_error_status()
{
  for(auto bulk : { false, true })
  {
    do_test_error_status(bulk);
  }
}

} // anonymous

int main()
{
  test_echo();
  test_error_status();
  
  return 0;
}
