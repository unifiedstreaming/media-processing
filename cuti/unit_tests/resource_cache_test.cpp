/*
 * Copyright (C) 2026 CodeShop B.V.
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

#include <cuti/resource_cache.hpp>

#include <cuti/cmdline_reader.hpp>
#include <cuti/logging_context.hpp>
#include <cuti/mutex_wrapper.hpp>
#include <cuti/option_walker.hpp>
#include <cuti/scoped_thread.hpp>
#include <cuti/streambuf_backend.hpp>

#include <algorithm>
#include <iostream>
#include <memory>
#include <thread>
#include <utility>

// enable assert
#undef NDEBUG
#include <cassert>

namespace // anonymous
{

using namespace cuti;

struct id_generator_t
{
  id_generator_t()
  : wrapped_next_(0)
  { }

  unsigned int next()
  {
    auto lock = wrapped_next_.lock();
    unsigned int result = *lock;
    ++(*lock);
    return result;
  }
    
private :
  mutex_wrapper_t<unsigned int> wrapped_next_;
};

struct resource_t
{
  resource_t(id_generator_t& generator,
             unsigned int key,
             logging_context_t const& context,
             char const* func)
  : id_(generator.next())
  , key_(key)
  , context_(context)
  , func_(func)
  {
    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << func_ << ": created resource# " << id_ <<
        " (key " << key_ << ')';
    }
  }

  resource_t(resource_t const&) = delete;
  resource_t& operator=(resource_t const&) = delete;
  
  unsigned int id() const
  { return id_; }

  unsigned int key() const
  { return key_; }

  ~resource_t()
  {
    if(auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << func_ << ": destroying resource# " << id_ <<
        " (key " << key_ << ')';
    }
  }

private :
  unsigned int id_;
  unsigned int key_;
  logging_context_t const& context_;
  char const* func_;
};

void check_different_ids(char const* func,
                         logging_context_t const& context,
                         resource_cache_settings_t const& settings)
{
  id_generator_t generator;
  
  auto ktor = [&generator, &context, func](unsigned int key)
  { return std::make_unique<resource_t>(generator, key, context, func); };

  auto rtok = [](resource_t const& resource)
  { return resource.key(); };

  resource_cache_t<unsigned int, resource_t> cache{ktor, rtok, settings};

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << func << ": excercising cache...";
  }

  auto resource1 = cache.obtain(42);
  assert(resource1->key() == 42);
  auto id1 = resource1->id();
  cache.store(std::move(resource1));

  auto resource2 = cache.obtain(42);
  assert(resource2->key() == 42);
  auto id2 = resource2->id();
  cache.store(std::move(resource2));
  
  assert(id1 != id2);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << func << ": done";
  }
}

void test_reuse(logging_context_t const& context)
{
  id_generator_t generator;
  char const* func = __func__;

  auto ktor = [&generator, &context, func](unsigned int key)
  { return std::make_unique<resource_t>(generator, key, context, func); };

  auto rtok = [](resource_t const& resource)
  { return resource.key(); };

  resource_cache_t<unsigned int, resource_t> cache{ktor, rtok};

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << func << ": excercising cache...";
  }

  auto resource1 = cache.obtain(42);
  assert(resource1->key() == 42);
  auto id1 = resource1->id();
  cache.store(std::move(resource1));

  auto resource2 = cache.obtain(42);
  assert(resource2->key() == 42);
  auto id2 = resource2->id();
  cache.store(std::move(resource2));
  
  assert(id1 == id2);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << func << ": done";
  }
}

void test_different_keys(logging_context_t const& context)
{
  id_generator_t generator;
  char const* func = __func__;

  auto ktor = [&generator, &context, func](unsigned int key)
  { return std::make_unique<resource_t>(generator, key, context, func); };

  auto rtok = [](resource_t const& resource)
  { return resource.key(); };

  resource_cache_t<unsigned int, resource_t> cache{ktor, rtok};

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << func << ": excercising cache...";
  }

  auto resource1 = cache.obtain(42);
  assert(resource1->key() == 42);
  auto id1 = resource1->id();
  cache.store(std::move(resource1));

  auto resource2 = cache.obtain(43);
  assert(resource2->key() == 43);
  auto id2 = resource2->id();
  cache.store(std::move(resource2));
  
  assert(id1 != id2);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << func << ": done";
  }
}

void test_mt_reuse(logging_context_t const& context)
{
  id_generator_t generator;
  char const* func = __func__;

  auto ktor = [&generator, &context, func](unsigned int key)
  { return std::make_unique<resource_t>(generator, key, context, func); };

  auto rtok = [](resource_t const& resource)
  { return resource.key(); };

  resource_cache_t<unsigned int, resource_t> cache{ktor, rtok};

  // populate cache
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << func << ": populating cache...";
  }

  {
    auto thread_body = [&cache]()
    {
      auto resource = cache.obtain(42);
      assert(resource->key() == 42);
      std::this_thread::yield();
      cache.store(std::move(resource));
    };

    std::vector<std::unique_ptr<scoped_thread_t>> threads;
    for(unsigned int i = 0; i != 10; ++i)
    {
      threads.push_back(std::make_unique<scoped_thread_t>(thread_body));
    }
  }

  // drain cache
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << func << ": draining cache...";
  }

  mutex_wrapper_t<std::vector<unsigned int>> ids{};
  {
    auto thread_body = [&ids, &cache]()
    {
      auto resource = cache.obtain(42);
      assert(resource->key() == 42);

      auto ids_lock = ids.lock();
      ids_lock->push_back(resource->id());
    };
    
    std::vector<std::unique_ptr<scoped_thread_t>> threads;
    for(unsigned int i = 0; i != 10; ++i)
    {
      threads.push_back(std::make_unique<scoped_thread_t>(thread_body));
    }
  }

  // check for unique ids
  {
    auto ids_lock = ids.lock();

    assert(ids_lock->size() == 10);
    std::sort(ids_lock->begin(), ids_lock->end());
    auto pos = std::adjacent_find(ids_lock->begin(), ids_lock->end());
    assert(pos == ids_lock->end());
  }

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << func << ": done";
  }
}    

void test_mt_different_keys(logging_context_t const& context)
{
  id_generator_t generator;
  char const* func = __func__;
  
  auto ktor = [&generator, &context, func](unsigned int key)
  { return std::make_unique<resource_t>(generator, key, context, func); };

  auto rtok = [](resource_t const& resource)
  { return resource.key(); };

  resource_cache_t<unsigned int, resource_t> cache{ktor, rtok};

  // populate cache
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << func << ": populating cache...";
  }

  {
    std::vector<std::unique_ptr<scoped_thread_t>> threads;
    for(unsigned int i = 0; i != 10; ++i)
    {
      auto thread_body = [&cache, key = i + 42]()
      {
        auto resource = cache.obtain(key);
        assert(resource->key() == key);
        std::this_thread::yield();
        cache.store(std::move(resource));
      };

      threads.push_back(std::make_unique<scoped_thread_t>(thread_body));
    }
  }

  // drain cache
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << func << ": draining cache...";
  }

  mutex_wrapper_t<std::vector<unsigned int>> ids{};
  {
    std::vector<std::unique_ptr<scoped_thread_t>> threads;
    for(unsigned int i = 0; i != 10; ++i)
    {
      auto thread_body = [&ids, &cache, key = i + 42]()
      {
        auto resource = cache.obtain(key);
        assert(resource->key() == key);

        auto ids_lock = ids.lock();
        ids_lock->push_back(resource->id());
      };
    
      threads.push_back(std::make_unique<scoped_thread_t>(thread_body));
    }
  }

  // check for unique ids
  {
    auto ids_lock = ids.lock();

    assert(ids_lock->size() == 10);
    std::sort(ids_lock->begin(), ids_lock->end());
    auto pos = std::adjacent_find(ids_lock->begin(), ids_lock->end());
    assert(pos == ids_lock->end());
  }

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << func << ": done";
  }
}

void test_max_keys(logging_context_t const& context)
{
  resource_cache_settings_t settings{};
  settings.max_keys_ = 0;

  check_different_ids(__func__, context, settings);
}

void test_max_key_age(logging_context_t const& context)
{
  resource_cache_settings_t settings{};
  settings.max_key_age_ = seconds_t{0};

  check_different_ids(__func__, context, settings);
}

void test_max_resources_per_key(logging_context_t const& context)
{
  resource_cache_settings_t settings{};
  settings.max_resources_per_key_ = 0;

  check_different_ids(__func__, context, settings);
}

void test_max_resource_age(logging_context_t const& context)
{
  resource_cache_settings_t settings{};
  settings.max_resource_age_ = seconds_t{0};

  check_different_ids(__func__, context, settings);
}

void test_wipe(logging_context_t const& context)
{
  id_generator_t generator;
  char const* func = __func__;
  
  auto ktor = [&generator, &context, func](unsigned int key)
  { return std::make_unique<resource_t>(generator, key, context, func); };

  auto rtok = [](resource_t const& resource)
  { return resource.key(); };

  resource_cache_t<unsigned int, resource_t> cache{ktor, rtok};

  // populate cache
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << func << ": populating cache...";
  }

  std::vector<std::unique_ptr<resource_t>> resources{};
  for(unsigned int i = 0; i != 10; ++i)
  {
    resources.push_back(cache.obtain(42));
  }

  std::vector<unsigned int> cached_ids{};
  while(!resources.empty())
  {
    cached_ids.push_back(resources.back()->id());
    cache.store(std::move(resources.back()));
    resources.pop_back();
  }

  // wipe key from cache
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << func << ": wiping key from cache...";
  }

  cache.wipe(42);

  // check for duplicates
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << func << ": checking for duplicates...";
  }

  for(unsigned int i = 0; i != 10; ++i)
  {
    auto resource = cache.obtain(42);
    auto pos = std::find(
      cached_ids.begin(), cached_ids.end(), resource->id());
    assert(pos == cached_ids.end());
  }

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << func << ": done";
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

  test_reuse(context);
  test_different_keys(context);
  test_mt_reuse(context);
  test_mt_different_keys(context);

  test_max_keys(context);
  test_max_key_age(context);
  test_max_resources_per_key(context);
  test_max_resource_age(context);

  test_wipe(context);
  
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
