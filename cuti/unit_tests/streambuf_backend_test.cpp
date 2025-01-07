/*
 * Copyright (C) 2021-2025 CodeShop B.V.
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

#include <cuti/streambuf_backend.hpp>
#include <cuti/scoped_thread.hpp>

#include <cstring>

#include <condition_variable>
#include <exception>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

// enable assert()
#undef NDEBUG
#include <cassert>

namespace // anonymous
{

struct thundering_herd_fence_t
{
  explicit thundering_herd_fence_t(unsigned int n_threads)
  : mutex_()
  , open_()
  , countdown_(n_threads)
  { }

  thundering_herd_fence_t(thundering_herd_fence_t const&) = delete;
  thundering_herd_fence_t& operator=(thundering_herd_fence_t const&) = delete;

  void pass()
  {
    bool did_open = false;
    {
      std::unique_lock<std::mutex> lock(mutex_);
      if(countdown_ != 0)
      {
        --countdown_;
        if(countdown_ == 0)
        {
          did_open = true;
        }
        else while(countdown_ != 0)
        {
          open_.wait(lock);
        }
      }
    }

    if(did_open)
    {
      open_.notify_all();
    }
  }

private :
  std::mutex mutex_;
  std::condition_variable open_;
  unsigned int countdown_;
};

void log_away(cuti::logger_t& logger, unsigned int n, unsigned int tid)
{
  for(unsigned int i = 0; i != n; ++i)
  {
    logger.report(cuti::loglevel_t::error,
      "event(e) " +  std::to_string(i) +
      " from thread <" + std::to_string(tid) + ">");
    logger.report(cuti::loglevel_t::warning,
      "event(w) " +  std::to_string(i) +
      " from thread <" + std::to_string(tid) + ">");
    logger.report(cuti::loglevel_t::info,
      "event(i) " +  std::to_string(i) +
      " from thread <" + std::to_string(tid) + ">");
    logger.report(cuti::loglevel_t::debug,
      "event(d) " +  std::to_string(i) +
      " from thread <" + std::to_string(tid) + ">");
  }
}

unsigned int count(std::string const& in, char const* search)
{
  unsigned int result = 0;

  std::string::size_type pos = in.find(search, 0);
  while(pos != std::string::npos)
  {
    ++result;
    pos += std::strlen(search);
    pos = in.find(search, pos);
  }

  return result;
}

unsigned int count_newlines(std::string const& in)
{
  return count(in, "\n");
}

unsigned int count_level(std::string const& in, cuti::loglevel_t level)
{
  return count(in, cuti::loglevel_string(level));
}

unsigned int count_tid(std::string const& in, unsigned int tid)
{
  std::string search = "from thread <" + std::to_string(tid) + ">";
  return count(in, search.c_str());
}

void test_single_threaded(char const* argv0)
{
  const unsigned int n_events = 100;

  std::stringstream strm;
  cuti::logger_t logger(std::make_unique<cuti::streambuf_backend_t>(strm));
  log_away(logger, n_events, 0);
  std::string s = strm.str();

#if 0
  std::cout << s << std::endl;
#endif

  assert(count_level(s, cuti::loglevel_t::error) == n_events);
  assert(count_level(s, cuti::loglevel_t::warning) == n_events);
  assert(count_level(s, cuti::loglevel_t::info) == n_events);
  assert(count_level(s, cuti::loglevel_t::debug) == n_events);

  assert(count_newlines(s) == 4 * n_events);
}

void test_multi_threaded(char const* argv0)
{
  const unsigned int n_threads = 10;
  const unsigned int n_events = 100;

  std::stringstream strm;
  cuti::logger_t logger(std::make_unique<cuti::streambuf_backend_t>(strm));

  thundering_herd_fence_t fence(n_threads);
  {
    std::vector<std::unique_ptr<cuti::scoped_thread_t>> threads;
    for(unsigned int tid = 0; tid != n_threads; ++tid)
    {
      auto code = [&, tid]
      {
        fence.pass();
        log_away(logger, n_events, tid);
      };

      threads.push_back(std::make_unique<cuti::scoped_thread_t>(code));
    }
  }

  std::string s = strm.str();

#if 0
  std::cout << s << std::endl;
#endif

  assert(count_level(s, cuti::loglevel_t::error) == n_events * n_threads);
  assert(count_level(s, cuti::loglevel_t::warning) == n_events * n_threads);
  assert(count_level(s, cuti::loglevel_t::info) == n_events * n_threads);
  assert(count_level(s, cuti::loglevel_t::debug) == n_events * n_threads);

  for(unsigned int tid = 0; tid != n_threads; ++tid)
  {
    assert(count_tid(s, tid) == 4 * n_events);
  }

  assert(count_newlines(s) == 4 * n_events * n_threads);
}

void run_tests(int, char const* const* argv)
{
  test_single_threaded(argv[0]);
  test_multi_threaded(argv[0]);
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
