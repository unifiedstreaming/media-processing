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

#include <cuti/circular_buffer.hpp>
#include <cuti/default_scheduler.hpp>
#include <cuti/logging_context.hpp>
#include <cuti/nb_inbuf.hpp>
#include <cuti/nb_outbuf.hpp>
#include <cuti/nb_string_inbuf.hpp>
#include <cuti/nb_string_outbuf.hpp>
#include <cuti/nb_tcp_buffers.hpp>
#include <cuti/streambuf_backend.hpp>

#include <iostream>
#include <tuple>

#undef NDEBUG
#include <cassert>

namespace // anoymous
{

using namespace cuti;

template<bool use_bulk_io>
struct copier_t
{
  copier_t(logging_context_t& context,
           scheduler_t& scheduler,
           std::unique_ptr<nb_inbuf_t> inbuf,
           std::unique_ptr<nb_outbuf_t> outbuf,
           std::size_t circ_bufsize)
  : context_(context)
  , scheduler_(scheduler)
  , inbuf_((assert(inbuf != nullptr), std::move(inbuf)))
  , outbuf_((assert(outbuf != nullptr), std::move(outbuf)))
  , circbuf_((assert(circ_bufsize != 0), circ_bufsize))
  , eof_seen_(false)
  { }

  copier_t(copier_t const&) = delete;
  copier_t& operator=(copier_t const&) = delete;
  
  void start()
  {
    inbuf_->call_when_readable(scheduler_, [this] { this->read_data(); });
  }

  bool done() const
  {
    return outbuf_ == nullptr;
  }

private :
  void read_data()
  {
    std::size_t bytes = 0;

    while(!eof_seen_ && circbuf_.has_slack() && inbuf_->readable())
    {
      if constexpr(use_bulk_io)
      {
        auto begin = circbuf_.begin_slack();
        auto next = inbuf_->read(begin, circbuf_.end_slack());
        if(next != begin)
        {
          circbuf_.push_back(next);

          bytes += next - begin;
        }
        else
        {
          eof_seen_ = true;
        }
      }
      else
      {
        int c = inbuf_->peek();
        if(c != eof)
        {
          inbuf_->skip();

          char *dst = circbuf_.begin_slack();
          *dst = static_cast<char>(c);
          circbuf_.push_back(dst + 1);

          ++bytes;
        }
        else
        {
          eof_seen_ = true;
        }
      }
    }

    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "copier@" << this << "::read_data(): " << bytes <<
        " byte(s) read (eof_seen_: " << eof_seen_ << ")";
    }

    if(!eof_seen_ && circbuf_.has_slack())
    {
      inbuf_->call_when_readable(scheduler_, [this] { this->read_data(); });
    }

    if(eof_seen_ || circbuf_.has_data())
    {
      outbuf_->call_when_writable(scheduler_, [this] { this->write_data(); });
    }
  }
      
  void write_data()
  {
    std::size_t bytes = 0;

    while(circbuf_.has_data() && outbuf_->writable())
    {
      if constexpr(use_bulk_io)
      {
        auto begin = circbuf_.begin_data();
        auto next = outbuf_->write(begin, circbuf_.end_data());
        circbuf_.pop_front(next);

        bytes += next - begin;    
      }
      else
      {
        char const* src = circbuf_.begin_data();
        outbuf_->put(*src);
        circbuf_.pop_front(src + 1);

        ++bytes;
      }
    }

    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "copier@" << this << "::write_data(): " << bytes <<
        " byte(s) written (eof_seen_: " << eof_seen_ << ")";
    }

    if(circbuf_.has_data())
    {
      outbuf_->call_when_writable(scheduler_, [this] { this->write_data(); });
    }

    if(!eof_seen_ && circbuf_.has_slack())
    {
      inbuf_->call_when_readable(scheduler_, [this] { this->read_data(); });
    }

    if(eof_seen_ && !circbuf_.has_data())
    {
      outbuf_->start_flush();
      outbuf_->call_when_writable(scheduler_, [this] { this->await_flush(); });
    }
  }

  void await_flush()
  {
    bool done = outbuf_->writable();

    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "copier@" << this << "::await_flush(): done: " << done;
    }

    if(!done)
    {
      outbuf_->call_when_writable(scheduler_, [this] { this->await_flush(); });
    }
    else
    {
      outbuf_ = nullptr;
    }
  }
  
private :
  logging_context_t& context_;
  scheduler_t& scheduler_;
  std::unique_ptr<nb_inbuf_t> inbuf_;
  std::unique_ptr<nb_outbuf_t> outbuf_;
  circular_buffer_t circbuf_;
  bool eof_seen_;
};

template<bool use_bulk_io>
void do_test_string_buffers(logging_context_t& context,
                            std::size_t circ_bufsize)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << "do_test_string_buffers(): use_bulk_io: " << use_bulk_io <<
      " circ_bufsize: " << circ_bufsize;
  }

  default_scheduler_t scheduler; 

  std::string input = "Hello peer";

  static constexpr std::size_t inbuf_sizes[] =
    { 1, 1, nb_inbuf_t::default_bufsize, nb_inbuf_t::default_bufsize };
  static constexpr std::size_t outbuf_sizes[] =
    { 1, nb_outbuf_t::default_bufsize, 1, nb_outbuf_t::default_bufsize };

  std::string outputs[4];
  std::unique_ptr<copier_t<use_bulk_io>> copiers[4];

  for(int i = 0; i != 4; ++i)
  {
    auto inbuf = make_nb_string_inbuf(input, inbuf_sizes[i]);
    auto outbuf = make_nb_string_outbuf(outputs[i], outbuf_sizes[i]);

    copiers[i] = std::make_unique<copier_t<use_bulk_io>>(
      context, scheduler, std::move(inbuf), std::move(outbuf), circ_bufsize);

    if(auto msg = context.message_at(loglevel_t::info))
    {
      *msg << "copier@" << copiers[i].get() << ": inbufsize: " <<
        inbuf_sizes[i] << " outbufsize: " << outbuf_sizes[i];
    }
  }

  for(auto& copier : copiers)
  {
    copier->start();
  }
    
  while(auto cb = scheduler.wait())
  {
    cb();
  }

  for(auto& copier : copiers)
  {
    assert(copier->done());
  }

  for(auto& output : outputs)
  {
    assert(output == input);
  }
}

void test_string_buffers(logging_context_t& context)
{
  do_test_string_buffers<false>(context, 1);
  do_test_string_buffers<false>(context, 16 * 1024);
  do_test_string_buffers<true>(context, 1);
  do_test_string_buffers<true>(context, 16 * 1024);
}
  
std::string make_large_payload()
{
  std::string result;

  unsigned int count = 0;
  while(result.size() < 1000 * 1000)
  {
    result += "Hello peer(";
    result += std::to_string(count);
    result += ") ";
    ++count;
  }

  return result;
}
  

template<bool use_bulk_io>
void do_test_tcp_buffers(logging_context_t& context,
                         std::size_t circ_bufsize,
                         std::size_t client_bufsize,
                         std::size_t server_bufsize,
                         std::string const& input)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << "do_test_tcp_buffers: use_bulk_io: " << use_bulk_io <<
      " circ_bufsize: " << circ_bufsize <<
      " client_bufsize: " << client_bufsize <<
      " server_bufsize: " << server_bufsize <<
      " payload: " << input.size() << " bytes";
  }

  default_scheduler_t scheduler;

  auto producer_in = make_nb_string_inbuf(input, client_bufsize);

  std::string output;
  auto consumer_out = make_nb_string_outbuf(output, client_bufsize);

  std::unique_ptr<tcp_connection_t> client_side;
  std::unique_ptr<tcp_connection_t> server_side;
  std::tie(client_side, server_side) = make_connected_pair();

  std::unique_ptr<nb_inbuf_t> consumer_in;
  std::unique_ptr<nb_outbuf_t> producer_out;
  std::tie(consumer_in, producer_out) = make_nb_tcp_buffers(
    std::move(client_side), client_bufsize, client_bufsize);

  std::unique_ptr<nb_inbuf_t> echoer_in;
  std::unique_ptr<nb_outbuf_t> echoer_out;
  std::tie(echoer_in, echoer_out) = make_nb_tcp_buffers(
    std::move(server_side), server_bufsize, server_bufsize);

  copier_t<use_bulk_io> producer(context, scheduler,
    std::move(producer_in), std::move(producer_out), circ_bufsize);
  copier_t<use_bulk_io> echoer(context, scheduler,
    std::move(echoer_in), std::move(echoer_out), circ_bufsize);
  copier_t<use_bulk_io> consumer(context, scheduler,
    std::move(consumer_in), std::move(consumer_out), circ_bufsize);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << "producer: copier@" << &producer <<
      " echoer: copier@" << &echoer <<
      " consumer: copier@" << &consumer;
  }

  producer.start();
  echoer.start();
  consumer.start();

  while(auto cb = scheduler.wait())
  {
    cb();
  }

  assert(producer.done());
  assert(echoer.done());
  assert(consumer.done());

  assert(input == output);
}

void test_tcp_buffers(logging_context_t& context)
{
  std::string const small_payload = "Hello peer";
  std::string const large_payload = make_large_payload();

  do_test_tcp_buffers<false>(context,
    1, 1, 1, small_payload);
  do_test_tcp_buffers<true>(context,
    1, 1, 1, small_payload);
  do_test_tcp_buffers<false>(context,
    128 * 1024, 256 * 1024, 256 * 1024, large_payload);
  do_test_tcp_buffers<true>(context,
    128 * 1024, 256 * 1024, 256 * 1024, large_payload);
  do_test_tcp_buffers<false>(context,
    256 * 1024, 128 * 1024, 128 * 1024, large_payload);
  do_test_tcp_buffers<true>(context,
    256 * 1024, 128 * 1024, 128 * 1024, large_payload);
  do_test_tcp_buffers<false>(context,
    256 * 1024, 256 * 1024, 256 * 1024, large_payload);
  do_test_tcp_buffers<true>(context,
    256 * 1024, 256 * 1024, 256 * 1024, large_payload);
}

void run_tests(int argc, char const* const*)
{
  logger_t logger(std::make_unique<streambuf_backend_t>(std::cerr));
  logging_context_t context(
    logger, argc == 1 ? loglevel_t::error : loglevel_t::info);

  test_string_buffers(context);
  test_tcp_buffers(context);
}

} // anonymous

int main(int argc, char* argv[])
{
  try
  {
    run_tests(argc, argv);
  }
  catch(std::exception const& ex)
  {
    std::cerr << argv[0] << ": exception: " << ex.what() << std::endl;
    throw;
  }

  return 0;
}
