/*
 * Copyright (C) 2021-2022 CodeShop B.V.
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
#include <cuti/cmdline_reader.hpp>
#include <cuti/default_scheduler.hpp>
#include <cuti/logging_context.hpp>
#include <cuti/nb_inbuf.hpp>
#include <cuti/nb_outbuf.hpp>
#include <cuti/nb_string_inbuf.hpp>
#include <cuti/nb_string_outbuf.hpp>
#include <cuti/nb_tcp_buffers.hpp>
#include <cuti/option_walker.hpp>
#include <cuti/streambuf_backend.hpp>
#include <cuti/system_error.hpp>

#include <iostream>
#include <sstream>
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
  , name_(make_name(*inbuf_, *outbuf_))
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
      *msg << "copier[" << *this << "]::read_data(): " << bytes <<
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
      *msg << "copier[" << *this << "]::write_data(): " << bytes <<
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
      *msg << "copier[" << *this << "]::await_flush(): done: " << done;
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

  friend std::ostream& operator<<(std::ostream& os, copier_t const& copier)
  {
    os << copier.name_;
    return os;
  }

private :
  static std::string make_name(nb_inbuf_t const& inbuf, nb_outbuf_t& outbuf)
  {
    std::ostringstream os;
    os << inbuf << " -> " << outbuf;
    return os.str();
  }
  
private :
  logging_context_t& context_;
  scheduler_t& scheduler_;
  std::unique_ptr<nb_inbuf_t> inbuf_;
  std::unique_ptr<nb_outbuf_t> outbuf_;
  circular_buffer_t circbuf_;
  bool eof_seen_;
  std::string name_;
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
      *msg << "copier[" << *copiers[i] << "]: inbufsize: " <<
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
    *msg << "producer: copier[" << producer <<
      "] echoer: copier[" << echoer <<
      "] consumer: copier[" << consumer << "]";
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

void drain(scheduler_t& scheduler, nb_inbuf_t& inbuf)
{
  while(inbuf.readable() && inbuf.peek() != eof) 
  {
    inbuf.skip();
  }

  if(!inbuf.readable())
  {
    inbuf.call_when_readable(scheduler, [&] { drain(scheduler, inbuf); });
  }
}

void drain_n(scheduler_t& scheduler, nb_inbuf_t& inbuf, std::size_t n)
{
  while(n != 0 && inbuf.readable() && inbuf.peek() != eof) 
  {
    inbuf.skip();
    --n;
  }

  if(n != 0 && !inbuf.readable())
  {
    inbuf.call_when_readable(scheduler,
      [&, n] { drain_n(scheduler, inbuf, n); });
  }
}

void flood(scheduler_t& scheduler, nb_outbuf_t& outbuf)
{
  while(outbuf.writable() && outbuf.error_status() == 0) 
  {
    outbuf.put('*');
  }

  if(!outbuf.writable())
  {
    outbuf.call_when_writable(scheduler, [&] { flood(scheduler, outbuf); });
  }
}

void flood_n(scheduler_t& scheduler, nb_outbuf_t& outbuf, std::size_t n)
{
  while(n != 0 && outbuf.writable() && outbuf.error_status() == 0) 
  {
    outbuf.put('*');
    if(--n == 0)
    {
      outbuf.start_flush();
    }
  }

  if(!outbuf.writable())
  {
    outbuf.call_when_writable(scheduler,
      [&, n] { flood_n(scheduler, outbuf, n); });
  }
}

void test_inbuf_throughput_checking(logging_context_t& context,
                                    bool enable_while_running,
                                    selector_factory_t const& factory)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << "test_inbuf_throughput_checking: enable_while_running: " <<
      enable_while_running << " selector: " << factory;
  }

  default_scheduler_t scheduler(factory);

  std::unique_ptr<tcp_connection_t> client_side;
  std::unique_ptr<tcp_connection_t> server_side;
  std::tie(client_side, server_side) = make_connected_pair();

  std::unique_ptr<nb_inbuf_t> client_in;
  std::unique_ptr<nb_outbuf_t> client_out;
  std::tie(client_in, client_out) =
    make_nb_tcp_buffers(std::move(client_side));

  std::unique_ptr<nb_inbuf_t> server_in;
  std::unique_ptr<nb_outbuf_t> server_out;
  std::tie(server_in, server_out) =
    make_nb_tcp_buffers(std::move(server_side));

  if(enable_while_running)
  {
    client_out->call_when_writable(scheduler,
      [&] { flood_n(scheduler, *client_out, 1234567); });
    server_in->call_when_readable(scheduler,
      [&] { drain(scheduler, *server_in); });

    server_in->enable_throughput_checking(512, 20, milliseconds_t(1));
  }
  else
  {
    server_in->enable_throughput_checking(512, 20, milliseconds_t(1));

    client_out->call_when_writable(scheduler,
      [&] { flood_n(scheduler, *client_out, 1234567); });
    server_in->call_when_readable(scheduler,
      [&] { drain(scheduler, *server_in); });
  }

  while(server_in->error_status() == 0)
  {
    auto cb = scheduler.wait();
    assert(cb != nullptr);
    cb();
  }

  assert(server_in->readable());
  assert(server_in->peek() == eof);
  assert(server_in->error_status() == timeout_system_error());
}

void test_outbuf_throughput_checking(logging_context_t& context,
                                     bool enable_while_running,
                                     selector_factory_t const& factory)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << "test_outbuf_throughput_checking: enable_while_running: " <<
      enable_while_running << " selector: " << factory;
  }

  default_scheduler_t scheduler(factory);

  std::unique_ptr<tcp_connection_t> client_side;
  std::unique_ptr<tcp_connection_t> server_side;
  std::tie(client_side, server_side) = make_connected_pair();

  std::unique_ptr<nb_inbuf_t> client_in;
  std::unique_ptr<nb_outbuf_t> client_out;
  std::tie(client_in, client_out) =
    make_nb_tcp_buffers(std::move(client_side));

  std::unique_ptr<nb_inbuf_t> server_in;
  std::unique_ptr<nb_outbuf_t> server_out;
  std::tie(server_in, server_out) =
    make_nb_tcp_buffers(std::move(server_side));

  if(enable_while_running)
  {
    client_out->call_when_writable(scheduler,
      [&] { flood(scheduler, *client_out); });
    server_in->call_when_readable(scheduler,
      [&] { drain_n(scheduler, *server_in, 1234567); });

    client_out->enable_throughput_checking(512, 20, milliseconds_t(1));
  }
  else
  {
    client_out->enable_throughput_checking(512, 20, milliseconds_t(1));

    client_out->call_when_writable(scheduler,
      [&] { flood(scheduler, *client_out); });
    server_in->call_when_readable(scheduler,
      [&] { drain_n(scheduler, *server_in, 1234567); });
  }

  while(client_out->error_status() == 0)
  {
    auto cb = scheduler.wait();
    assert(cb != nullptr);
    cb();
  }

  assert(client_out->writable());
  assert(client_out->error_status() == timeout_system_error());
}

void test_throughput_checking(logging_context_t& context)
{
  for(auto const& factory : available_selector_factories())
  {
    test_inbuf_throughput_checking(context, false, factory);
    test_inbuf_throughput_checking(context, true, factory);

    test_outbuf_throughput_checking(context, false, factory);
    test_outbuf_throughput_checking(context, true, factory);
  }
}
  
struct options_t
{
  static loglevel_t constexpr default_loglevel = loglevel_t::error;

  options_t()
  : loglevel_(default_loglevel)
  { }

  loglevel_t loglevel_;
};

void print_usage(std::ostream& os, char const* argv0)
{
  os << "usage: " << argv0 << " [<option> ...]\n";
  os << "options are:\n";
  os << "  --loglevel <level>       set loglevel " <<
    "(default: " << loglevel_string(options_t::default_loglevel) << ")\n";
  os << std::flush;
}

void read_options(options_t& options, option_walker_t& walker)
{
  while(!walker.done())
  {
    if(!walker.match("--loglevel", options.loglevel_))
    {
      break;
    }
  }
}

int run_tests(int argc, char const* const* argv)
{
  options_t options;
  cmdline_reader_t reader(argc, argv);
  option_walker_t walker(reader);

  read_options(options, walker);
  if(!walker.done() || !reader.at_end())
  {
    print_usage(std::cerr, argv[0]);
    return 1;
  }

  logger_t logger(std::make_unique<streambuf_backend_t>(std::cerr));
  logging_context_t context(logger, options.loglevel_);
  
  test_string_buffers(context);
  test_tcp_buffers(context);
  test_throughput_checking(context);

  return 0;
}

} // anonymous

int main(int argc, char* argv[])
{
  try
  {
    return run_tests(argc, argv);
  }
  catch(std::exception const& ex)
  {
    std::cerr << argv[0] << ": exception: " << ex.what() << std::endl;
  }

  return 1;
}
