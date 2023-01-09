/*
 * Copyright (C) 2022-2023 CodeShop B.V.
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

#include <cuti/event_pipe.hpp>

#include <cuti/cmdline_reader.hpp>
#include <cuti/default_scheduler.hpp>
#include <cuti/logging_context.hpp>
#include <cuti/option_walker.hpp>
#include <cuti/scoped_thread.hpp>
#include <cuti/streambuf_backend.hpp>

#include <iostream>
#include <condition_variable>
#include <mutex>
#include <utility>

#undef NDEBUG
#include <cassert>

using namespace cuti;

namespace // anonymous
{

struct event_producer_t
{
  event_producer_t(std::unique_ptr<event_pipe_writer_t> writer,
                   scheduler_t& scheduler,
                   unsigned int count)
  : writer_(std::move(writer))
  , scheduler_(scheduler)
  , count_(count)
  , ticket_()
  {
    this->proceed();
  }

  event_producer_t(event_producer_t const&) = delete;
  event_producer_t& operator=(event_producer_t const&) = delete;

  bool done() const
  {
    return count_ == 0;
  }

  ~event_producer_t()
  {
    if(!ticket_.empty())
    {
      scheduler_.cancel(ticket_);
    }
  }

private :
  void proceed()
  {
    if(count_ != 0)
    {
      ticket_ = writer_->call_when_writable(scheduler_,
        [this] { this->on_writable(); });
    }
    else
    {
      writer_.reset();
    }
  }    

  void on_writable()
  {
    assert(!ticket_.empty());
    ticket_.clear();

    assert(count_ != 0);

    if(writer_->write(static_cast<unsigned char>(count_)))
    {
      --count_;
    }

    this->proceed();
  }

private :
  std::unique_ptr<event_pipe_writer_t> writer_;
  scheduler_t& scheduler_;
  unsigned int count_;
  cancellation_ticket_t ticket_;
};

struct event_consumer_t
{
  event_consumer_t(std::unique_ptr<event_pipe_reader_t> reader,
                   scheduler_t& scheduler,
                   unsigned int count)
  : reader_(std::move(reader))
  , scheduler_(scheduler)
  , count_(count)
  , eof_seen_(false)
  , ticket_()
  {
    ticket_ = reader_->call_when_readable(scheduler_,
      [ this ] { this->on_readable(); });
  }

  event_consumer_t(event_consumer_t const&) = delete;
  event_consumer_t& operator=(event_consumer_t const&) = delete;

  bool done() const
  {
    return eof_seen_;
  }

  ~event_consumer_t()
  {
    if(!ticket_.empty())
    {
      scheduler_.cancel(ticket_);
    }
  }

private :
  void on_readable()
  {
    assert(!ticket_.empty());
    ticket_.clear();

    std::optional<int> r = reader_->read();
    if(r != std::nullopt)
    {
      if(count_ == 0)
      {
        assert(*r == eof);
        eof_seen_ = true;
      }
      else
      {
        assert(*r == static_cast<int>(count_ % 256));
        --count_;
      }
    }

    if(!eof_seen_)
    {
      ticket_ = reader_->call_when_readable(scheduler_,
        [ this ] { this->on_readable(); });
    }
  }        

private :
  std::unique_ptr<event_pipe_reader_t> reader_;
  scheduler_t& scheduler_;
  unsigned int count_;
  bool eof_seen_;
  cancellation_ticket_t ticket_;
};

/*
 * A simple scheduler-based multiple consumer queue.
 */
struct mcq_t
{
  explicit mcq_t(selector_factory_t const& factory)
  : mutex_()
  , active_(false)
  , check_active_()
  , reader_(nullptr)
  , writer_(nullptr)
  , scheduler_(factory)
  , selecting_(false)
  , check_selecting_()
  {
    std::tie(reader_, writer_) = make_event_pipe();
  }

  mcq_t(mcq_t const&) = delete;
  mcq_t& operator=(mcq_t const&) = delete;
  
  void push(int c)
  {
    std::unique_lock<std::mutex> lock(mutex_);

    while(active_)
    {
      assert(writer_ != nullptr);
      check_active_.wait(lock);
    }

    assert(writer_ != nullptr);

    if(c != eof)
    {
      writer_->write(static_cast<unsigned char>(c));
    }
    else
    {
      writer_.reset();
    }
      
    active_ = true;
  }

  int pull()
  {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      while(selecting_)
      {
        check_selecting_.wait(lock);
      }
      selecting_ = true;
    }

    std::optional<int> rr = std::nullopt;
    do
    {
      bool readable = false;
      reader_->call_when_readable(scheduler_, [&] { readable = true; });
      do 
      {
        auto cb = scheduler_.wait();
        assert(cb != nullptr);
        cb();
      } while(!readable);

      {
        std::unique_lock<std::mutex> lock(mutex_);
        if(active_)
        {
          rr = reader_->read();
          assert(rr != std::nullopt);

          if(*rr != eof)
          {
            active_ = false;
          }
          selecting_ = false;
        }
        else
        {
          // spurious wakeup: retry
        }
      }
    } while(rr == std::nullopt);

    check_active_.notify_all();
    check_selecting_.notify_one();

    return *rr;
  }

private :
  mutable std::mutex mutex_;
  bool active_;
  std::condition_variable check_active_;
  std::unique_ptr<event_pipe_reader_t> reader_; // readable when active
  std::unique_ptr<event_pipe_writer_t> writer_; // nullptr when closed
  default_scheduler_t scheduler_;
  bool selecting_;
  std::condition_variable check_selecting_;
};

void blocking_transfer(logging_context_t const& context)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  std::unique_ptr<event_pipe_reader_t> reader;
  std::unique_ptr<event_pipe_writer_t> writer;

  std::tie(reader, writer) = make_event_pipe();

  for(int i = 0; i != 256; ++i)
  {
    assert(writer->write(static_cast<unsigned char>(i)));
    auto r = reader->read();
    assert(r != std::nullopt);
    assert(*r == i);
  }

  writer.reset();
  auto r = reader->read();
  assert(r != std::nullopt);
  assert(*r == eof);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}
  
void nonblocking_transfer(logging_context_t const& context)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  std::unique_ptr<event_pipe_reader_t> reader;
  std::unique_ptr<event_pipe_writer_t> writer;

  std::tie(reader, writer) = make_event_pipe();
  reader->set_nonblocking();
  writer->set_nonblocking();
  
  std::optional<int> r;

  unsigned int write_spins = 0;
  unsigned int read_spins = 0;

  for(int i = 0; i != 256; ++i)
  {
    while(writer->write(static_cast<unsigned char>(i)) == false)
    {
      ++write_spins;
    }

    while((r = reader->read()) == std::nullopt)
    {
      ++read_spins;
    }

    assert(*r == i);
  }

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": write spins in loop: " << write_spins <<
      " read spins in loop: " << read_spins;
  }

  r = reader->read();
  assert(r == std::nullopt);

  writer.reset();

  read_spins = 0;
  while((r = reader->read()) == std::nullopt)
  {
    ++read_spins;
  }

  assert(*r == eof);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": read spins expecting eof: " << read_spins;
  }

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}

void scheduled_transfer(logging_context_t const& context,
                        selector_factory_t const& factory,
                        unsigned int count)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting; selector: " << factory <<
      " count: " << count;
  }

  std::unique_ptr<event_pipe_reader_t> reader;
  std::unique_ptr<event_pipe_writer_t> writer;

  std::tie(reader, writer) = make_event_pipe();
  reader->set_nonblocking();
  writer->set_nonblocking();

  default_scheduler_t scheduler;

  event_consumer_t consumer(std::move(reader), scheduler, count);
  event_producer_t producer(std::move(writer), scheduler, count);

  unsigned int n_callbacks = 0;
  while(!consumer.done())
  {
    auto cb = scheduler.wait();
    assert(cb != nullptr);
    cb();
    ++n_callbacks;
  }

  assert(producer.done());
  assert(scheduler.wait() == nullptr);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done; n_callbacks: " << n_callbacks;
  }
}

void scheduled_transfer(logging_context_t const& context)
{
  auto factories = available_selector_factories();
  unsigned int constexpr counts[] = { 0, 256, 30000 };

  for(auto const& factory : factories)
  {
    for(auto count : counts)
    {
      scheduled_transfer(context, factory, count);
    }
  }
}

unsigned int pull_mcq(logging_context_t const& context,
                      int tid,
                      mcq_t& queue)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << "(tid " << tid << "): pulling";
  }

  unsigned int count = 0;

  int pulled;
  while((pulled = queue.pull()) != eof)
  {
    assert(pulled == '*');
    ++count;
  }

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << "(tid " << tid <<
      "): pulling done (eof after reading " << count << " events)";
  }

  return count;
}

void multiple_consumer_queue(logging_context_t const& context,
                             selector_factory_t const& factory)
{
  static unsigned int constexpr n_threads = 17;
  static unsigned int constexpr n_events = n_threads * 100;

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting (selector: " << factory << ")";
  }

  mcq_t queue(factory);
  unsigned int counts[n_threads];
  {
    std::unique_ptr<scoped_thread_t> threads[n_threads];
    auto guard = make_scoped_guard([&] { queue.push(eof); });

    for(unsigned int tid = 0; tid != n_threads; ++tid)
    {
      threads[tid] = std::make_unique<scoped_thread_t>(
        [&, tid] { counts[tid] = pull_mcq(context, tid, queue); }); 
    }

    for(unsigned int i = 0; i != n_events; ++i)
    {
      queue.push('*');
    }

    if(auto msg = context.message_at(loglevel_t::info))
    {
      *msg << __func__ << ": pushing done";
    }
  }

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": threads joined";
  }

  unsigned int total_count = 0;
  for(unsigned int i = 0; i != n_threads; ++i)
  {
    total_count += counts[i];
  }
  assert(total_count == n_events);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}

void multiple_consumer_queue(logging_context_t const& context)
{
  auto factories = available_selector_factories();
  for(auto const& factory : factories)
  {
    multiple_consumer_queue(context, factory);
  }
}

void do_run_tests(logging_context_t const& context)
{
  blocking_transfer(context);
  nonblocking_transfer(context);
  scheduled_transfer(context);
  multiple_consumer_queue(context);
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
    if(!walker.match(
         "--loglevel", options.loglevel_))
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

  do_run_tests(context);

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
