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
#include <cuti/default_scheduler.hpp>
#include <cuti/scheduler.hpp>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <variant>

#undef NDEBUG
#include <cassert>

namespace // anonymous
{

using namespace cuti;

struct async_source_t
{
  static int constexpr eof = async_inbuf_t::eof;

  async_source_t(async_inbuf_t& inbuf, scheduler_t& scheduler)
  : inbuf_(inbuf)
  , scheduler_(scheduler)
  { }

  bool readable() const
  {
    return inbuf_.readable();
  }

  int peek() const
  {
    return inbuf_.peek();
  }

  void skip()
  {
    inbuf_.skip();
  }

  char* read(char* first, char const* last)
  {
    return inbuf_.read(first, last);
  }

  void call_when_readable(callback_t callback)
  {
    inbuf_.call_when_readable(scheduler_, std::move(callback));
  }

private :
  async_inbuf_t& inbuf_;
  scheduler_t& scheduler_;
};

struct async_array_input_t : async_input_t
{
  template<int N>
  explicit async_array_input_t(char const (&arr)[N])
  : rp_(arr)
  , ep_(arr + N - 1)
  , readable_holder_()
  { }

  void call_when_readable(scheduler_t& scheduler, callback_t callback) override
  {
    readable_holder_.call_alarm(
      scheduler, duration_t::zero(), std::move(callback));
  }

  void cancel_when_readable() noexcept override
  {
    readable_holder_.cancel();
  }

  char* read(char* first, char const* last) override
  {
    auto count = std::min(ep_ - rp_, last - first);
    std::copy(rp_, rp_ + count, first);
    rp_ += count;
    return first + count;
  }

  int error_status() const noexcept override
  {
    return 0;
  }
  
private :
  char const* rp_;
  char const* const ep_;
  ticket_holder_t readable_holder_;  
};

template<typename Function, typename Next>
struct link_t
{
  link_t(Function function, Next next)
  : function_(function)
  , next_(next)
  { }

  template<typename... Args>
  void start(async_source_t source, Args... args)
  {
    function_(source, next_, args...);
  }

  void fail(char const* error)
  {
    next_.fail(error);
  }
    
private :
  Function function_;
  Next next_;
};

template<typename Function, typename Next>
auto make_link(Function function, Next next)
{
  return link_t<Function, Next>(function, next);
}

template<typename Last>
auto make_engine(Last last)
{
  return last;
}

template<typename First, typename Second, typename... Rest>
auto make_engine(First first, Second second, Rest... rest)
{
  return make_link(first, make_engine(second, rest...));
}

template<typename T>
struct result_t
{
  result_t()
  : shared_state_(std::make_shared<shared_state_t>())
  { }

  bool available() const
  {
    return shared_state_->index() != 0;
  }

  char const* error() const
  {
    assert(this->available());
    return shared_state_->index() == 2 ? std::get<2>(*shared_state_) : nullptr;
  }

  T value() const
  {
    assert(this->error() == nullptr);
    return std::get<1>(*shared_state_);
  }

  void start(async_source_t, T arg)
  {
    assert(!this->available());
    shared_state_->template emplace<1>(arg);
  }

  void fail(char const* error)
  {
    assert(error != nullptr);
    assert(!this->available());
    shared_state_->template emplace<2>(error);
  }

private :
  using shared_state_t =
    std::variant<std::monostate, T, char const*>;
  std::shared_ptr<shared_state_t> shared_state_;
};

template<>
struct result_t<void>
{
  result_t()
  : shared_state_(std::make_shared<shared_state_t>())
  { }

  bool available() const
  {
    return shared_state_->available_;
  }

  char const* error() const
  {
    assert(this->available());
    return shared_state_->error_;
  }

  void value() const
  {
    assert(this->error() == nullptr);
  }

  void start(async_source_t)
  {
    assert(!this->available());
    shared_state_->available_ = true;
  }

  void fail(char const* error)
  {
    assert(error != nullptr);
    assert(!this->available());
    shared_state_->available_ = true;
    shared_state_->error_ = error;
  }

private :
  struct shared_state_t
  {
    shared_state_t()
    : available_(false)
    , error_(nullptr)
    { }

    bool available_;
    char const* error_;
  };

  std::shared_ptr<shared_state_t> shared_state_;
};

auto inline constexpr
force_error = [](auto, auto next, auto...)
{
  next.fail("forced error");
};

template<typename Next, typename... Args>
void read_eof_impl(async_source_t source, Next next, Args... args)
{
  if(!source.readable())
  {
    source.call_when_readable([=] { read_eof_impl(source, next, args...); });
    return;
  }

  if(source.peek() != async_source_t::eof)
  {
    next.fail("eof expected");
  }
  else
  {
    next.start(source, args...);
  }
};

auto inline constexpr
read_eof = [](auto... args) { read_eof_impl(args...); };

inline bool is_space(int c)
{
  return c == '\t' || c == '\r' || c == ' ';
}

template<typename Next, typename... Args>
void skip_spaces_impl(async_source_t source, Next next, Args... args)
{
  while(source.readable() && is_space(source.peek()))
  {
    source.skip();
  }

  if(!source.readable())
  {
    source.call_when_readable([=] { skip_spaces_impl(source, next, args...); });
    return;
  }

  next.start(source, args...);
}

auto inline constexpr
skip_spaces = [](auto... args) { skip_spaces_impl(args...); };

template<typename Engine, int N, typename... Args>
void run_engine(Engine engine, char const (&input)[N], Args... args)
{
  async_inbuf_t inbuf(std::make_unique<async_array_input_t>(input), 1);
  default_scheduler_t scheduler;
  async_source_t source(inbuf, scheduler);

  engine.start(source, args...);

  while(auto callback = scheduler.wait())
  {
    callback();
  }
}
  
void test_result()
{
  {
    result_t<void> result;
    assert(!result.available());
    
    auto engine = make_engine(result);
    run_engine(make_engine(result), "");

    assert(result.available());
    assert(result.error() == nullptr);
    result.value();
  }

  {
    result_t<void> result;
    assert(!result.available());
    
    auto engine = make_engine(force_error, result);
    run_engine(engine, "");

    assert(result.available());
    assert(result.error() != nullptr);
    assert(std::strcmp(result.error(), "forced error") == 0);
  }

  {
    result_t<int> result;
    assert(!result.available());
    
    auto engine = make_engine(force_error, result);
    run_engine(engine, "", 42);

    assert(result.available());
    assert(result.error() != nullptr);
    assert(std::strcmp(result.error(), "forced error") == 0);
  }
}

void test_read_eof()
{
  {
    result_t<void> result;
    auto engine = make_engine(read_eof, result);
    run_engine(engine, "");
    result.value();
  }

  {
    result_t<void> result;
    auto engine = make_engine(read_eof, result);
    run_engine(engine, " ");
    assert(result.error() != nullptr);
  }

  {
    result_t<int> result;
    auto engine = make_engine(read_eof, result);
    run_engine(engine, "", 42);
    assert(result.value() == 42);
  }

  {
    result_t<int> result;
    auto engine = make_engine(read_eof, result);
    run_engine(engine, " ", 42);
    assert(result.error() != nullptr);
  }
}
    
void test_skip_spaces()
{
  {
    result_t<void> result;
    auto engine = make_engine(skip_spaces, read_eof, result);
    run_engine(engine, "\t\r ");
    result.value();
  }

  {
    result_t<void> result;
    auto engine = make_engine(skip_spaces, read_eof, result);
    run_engine(engine, "");
    result.value();
  }

  {
    result_t<int> result;
    auto engine = make_engine(skip_spaces, read_eof, result);
    run_engine(engine, " \r\t", 42);
    assert(result.value() == 42);
  }

  {
    result_t<int> result;
    auto engine = make_engine(skip_spaces, read_eof, result);
    run_engine(engine, "", 42);
    assert(result.value() == 42);
  }
}
    
} // anonymous

int main()
{
  test_result();
  test_read_eof();
  test_skip_spaces();

  return 0;
}
