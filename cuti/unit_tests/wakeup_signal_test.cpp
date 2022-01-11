/*
 * Copyright (C) 2022 CodeShop B.V.
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

#include <cuti/wakeup_signal.hpp>

#include <cuti/cmdline_reader.hpp>
#include <cuti/default_scheduler.hpp>
#include <cuti/logging_context.hpp>
#include <cuti/option_walker.hpp>
#include <cuti/scoped_thread.hpp>
#include <cuti/streambuf_backend.hpp>

#include <iostream>

#undef NDEBUG
#include <cassert>

namespace // anonymous
{

using namespace cuti;

int constexpr n_threads = 17;

void await_wakeup(logging_context_t const& context,
                  int thread_id,
                  wakeup_signal_t const& signal,
                  selector_factory_t const& selector_factory)
{
  default_scheduler_t scheduler(selector_factory);
  wakeup_signal_watcher_t watcher(signal, scheduler);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": (thread " << thread_id << "): waiting...";
  }

  bool woken_up = false;
  watcher.call_when_active([&] { woken_up = true; });

  while(!woken_up)
  {
    auto cb = scheduler.wait();
    assert(cb != nullptr);
    cb();
  }
  
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": (thread " << thread_id << "): woken up";
  }
}

void test_wakeup(logging_context_t const& context,
                 selector_factory_t const& selector_factory)
{
  wakeup_signal_t signal;

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": (selector " << selector_factory <<
      "): spawning threads...";
  }

  {
    std::unique_ptr<scoped_thread_t> threads[n_threads];
    for(int i = 0; i != n_threads; ++i)
    {
      threads[i] = std::make_unique<scoped_thread_t>(
        [&, i] { await_wakeup(context, i, signal, selector_factory); });
    }

    if(auto msg = context.message_at(loglevel_t::info))
    {
      *msg << __func__ << ": activating signal";
    }

    signal.activate();
    assert(signal.active());
  }

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": threads joined";
  }

  signal.deactivate();
  assert(!signal.active());
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

  auto factories = available_selector_factories();
  for(auto const& factory : factories)
  {
    test_wakeup(context, factory);
  }
  
  return 0;
}
  
} // anonymous
                  
int main(int argc, char* argv[])
{
  int r = 1;

  try
  {
    r = run_tests(argc, argv);
  }
  catch(std::exception const& ex)
  {
    std::cerr << argv[0] << ": exception: " << ex.what() << std::endl;
    throw;
  }

  return r;
}
