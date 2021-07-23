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
#include <cuti/oneshot.hpp>
#include <cuti/scheduler.hpp>

#include <algorithm>
#include <cstring>
#include <functional>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>
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

  async_source_t(async_source_t const&) = delete;

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

template<typename Function, typename Next>
struct link_t
{
  link_t(Function function, Next next)
  : function_(function)
  , next_(next)
  { }

  template<typename... Args>
  void start(async_source_t& source, Args&&... args)
  {
    function_(source, next_, std::forward<Args>(args)...);
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

template<typename A1, typename A2, typename A3, typename... Rest>
auto make_link(A1 a1, A2 a2, A3 a3, Rest... rest)
{
  return make_link(a1, make_link(a2, a3, rest...));
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

template<typename F1, typename F2>
struct combine_t
{
  combine_t(F1 f1, F2 f2)
  : f1_(f1)
  , f2_(f2)
  { }

  template<typename Next, typename... Args>
  void operator()(async_source_t& source, Next next, Args&&... args)
  {
    auto link = make_link(f2_, next);
    f1_(source, link, std::forward<Args>(args)...);
  }
    
private :
  F1 f1_;
  F2 f2_;
};

template<typename F1, typename F2>
auto combine(F1 f1, F2 f2)
{
  return combine_t<F1, F2>(f1, f2);
}

template<typename F1, typename F2, typename F3, typename... Fn>
auto combine(F1 f1, F2 f2, F3 f3, Fn... fn)
{
  return combine(f1, combine(f2, f3, fn...));
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

  void start(async_source_t&, T arg)
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

  void start(async_source_t&)
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

struct read_eof_t
{
  template<typename Next, typename... Args>
  void operator()(async_source_t& source, Next next, Args&&... args) const
  {
    if(!source.readable())
    {
      source.call_when_readable(make_oneshot(
        *this, std::ref(source), next, std::forward<Args>(args)...));
      return;
    }

    if(source.peek() != async_source_t::eof)
    {
      next.fail("eof expected");
      return;
    }

    next.start(source, std::forward<Args>(args)...);
  }
};

auto inline constexpr read_eof = read_eof_t{};

inline bool is_space(int c)
{
  return c == '\t' || c == '\r' || c == ' ';
}

struct skip_spaces_t
{
  template<typename Next, typename... Args>
  void operator()(async_source_t& source, Next next, Args&&... args) const
  {
    while(source.readable() && is_space(source.peek()))
    {
      source.skip();
    }

    if(!source.readable())
    {
      source.call_when_readable(make_oneshot(
        *this, std::ref(source), next, std::forward<Args>(args)...));
      return;
    }

    next.start(source, std::forward<Args>(args)...);
  }

};

auto inline constexpr skip_spaces = skip_spaces_t{};

auto constexpr invalid_digit = std::numeric_limits<unsigned char>::max(); 

unsigned char digit_value(int c)
{
  if(c < '0' || c > '9')
  {
    return invalid_digit;
  }

  return c - '0';
}
    
struct read_first_digit_t
{
  template<typename Next, typename... Args>
  void operator()(async_source_t& source, Next next, Args&&... args) const
  {
    if(!source.readable())
    {
      source.call_when_readable(make_oneshot(
        *this, std::ref(source), next, std::forward<Args>(args)...));
      return;
    }

    unsigned int dval = digit_value(source.peek());
    if(dval == invalid_digit)
    {
      next.fail("digit expected");
      return;
    }

    source.skip();
    next.start(source, dval, std::forward<Args>(args)...);
  }
};

auto inline constexpr read_first_digit = read_first_digit_t{};
  
template<typename T>
struct read_trailing_digits_t
{
  template<typename Next, typename... Args>
  void operator()(
    async_source_t& source, Next next, T total, T limit, Args&&... args) const
  {
    static_assert(std::is_unsigned_v<T>);

    unsigned char dval;
    while(source.readable() &&
          (dval = digit_value(source.peek())) != invalid_digit)
    {
      if(total > limit / 10U || dval > limit - total * 10U)
      {
        next.fail("integral value overflow");
        return;
      }

      total *= 10;
      total += dval;
      source.skip();
    }

    if(!source.readable())
    {
      source.call_when_readable(make_oneshot(*this, std::ref(source), next,
        total, limit, std::forward<Args>(args)...));
      return;
    }

    next.start(source, total, std::forward<Args>(args)...);
  }
};

template<typename T>
auto inline constexpr read_trailing_digits = read_trailing_digits_t<T>{};

template<typename T>
struct read_unsigned_t
{
  template<typename Next, typename... Args>
  void operator()(async_source_t& source, Next next, Args&&... args) const
  {
    static_assert(std::is_unsigned_v<T>);

    auto c = combine(skip_spaces, read_first_digit, read_trailing_digits<T>);
    return c(source, next, std::numeric_limits<T>::max(),
      std::forward<Args>(args)...);
  }
};

template<typename T>
auto inline constexpr read_unsigned = read_unsigned_t<T>{};

struct read_optional_sign_t
{
  template<typename Next, typename... Args>
  void operator()(async_source_t& source, Next next, Args&&... args) const
  {
    if(!source.readable())
    {
      source.call_when_readable(make_oneshot(*this, std::ref(source), next,
        std::forward<Args>(args)...));
      return;
    }

    bool result;
    switch(source.peek())
    {
    case '+' :
      result = false;
      source.skip();
      break;
    case '-' :
      result = true;
      source.skip();
      break;
    default :
      result = false;
      break;
    }

    next.start(source, result, args...);
  }
};

auto inline constexpr read_optional_sign = read_optional_sign_t{};

template<typename T>
struct insert_limit_t
{
  template<typename Next, typename... Args>
  void operator()(async_source_t& source, Next next, bool negative,
    Args&&... args) const
  {
    static_assert(std::is_signed_v<T>);
    std::make_unsigned_t<T> limit = std::numeric_limits<T>::max();
    if(negative)
    {
      ++limit;
    }
    next.start(source, limit, negative, std::forward<Args>(args)...);
  }
};

template<typename T>
auto inline constexpr insert_limit = insert_limit_t<T>();

template<typename T>
struct to_signed_t
{
  template<typename Next, typename... Args>
  void operator()(async_source_t& source, Next next, T value, bool negative,
    Args&&... args) const
  {
    static_assert(std::is_unsigned_v<T>);
    
    std::make_signed_t<T> result;
    if(negative && value != 0)
    {
      --value;
      result = value;
      result = -result;
      --result;
    }
    else
    {
      result = value;
    }
    next.start(source, result, std::forward<Args>(args)...);
  }
};
  
template<typename T>
auto inline constexpr to_signed = to_signed_t<T>{};

template<typename T>
struct read_signed_t
{
  template<typename Next, typename... Args>
  void operator()(async_source_t& source, Next next, Args&&... args) const
  {
    static_assert(std::is_signed_v<T>);
    using UT = std::make_unsigned_t<T>;

    auto c = combine(skip_spaces, read_optional_sign, insert_limit<T>,
      read_first_digit, read_trailing_digits<UT>, to_signed<UT>);
    return c(source, next, std::forward<Args>(args)...);
  }
};

template<typename T>
auto inline constexpr read_signed = read_signed_t<T>{};

template<typename T>
auto constexpr make_read_integral()
{
  static_assert(std::is_integral_v<T>);
  if constexpr(std::is_unsigned_v<T>)
  {
    return read_unsigned<T>;
  }
  else
  {
    return read_signed<T>;
  }
}
  
template<typename T>
auto inline constexpr read_integral = make_read_integral<T>();

template<typename T>
struct append_element_t
{
  template<typename Next, typename... Args>
  void operator()(async_source_t& source, Next next,
    T element, std::vector<T> elements,
    Args&&... args) const
  {
    elements.push_back(element);
    next.start(source, std::move(elements), std::forward<Args>(args)...);
  }
};

template<typename T>
auto inline constexpr append_element = append_element_t<T>{};
  
int constexpr max_recursion = 100;

template<typename T>
struct append_elements_t
{
  template<typename Next, typename... Args>
  void operator()(async_source_t& source, Next next,
                  std::vector<T> elements, int recursion,
		  Args&&... args) const
  {
    int c;
    while(source.readable() &&
      recursion != max_recursion &&
      is_space(c = source.peek()))
    {
      source.skip();
    }
  
    if(!source.readable() || recursion == max_recursion)
    {
      source.call_when_readable(make_oneshot(
        *this, std::ref(source), next,
        std::move(elements), 0, std::forward<Args>(args)...));
      return;
    }

    if(c != ']')
    {
      auto c = combine(read_integral<T>, append_element<T>, *this);
      c(source, next, std::move(elements), recursion + 1,
        std::forward<Args>(args)...);
      return;
    }

    source.skip();
    next.start(source, std::move(elements), std::forward<Args>(args)...);
  }
};
      
template<typename T>
auto inline constexpr append_elements = append_elements_t<T>{};

template<typename T>
struct read_vector_t
{
  template<typename Next, typename... Args>
  void operator()(async_source_t& source, Next next, Args&&... args) const
  {
    int c;
    while(source.readable() && is_space(c = source.peek()))
    {
      source.skip();
    }

    if(!source.readable())
    {
      source.call_when_readable(make_oneshot(
        *this, std::ref(source), next, std::forward<Args>(args)...));
      return;
    }

    if(c != '[')
    {
      next.fail("'[' expected");
      return;
    }

    source.skip();
    append_elements<T>(
      source, next, std::vector<T>(), 0, std::forward<Args>(args)...);
  }
};

template<typename T>
auto inline constexpr read_vector = read_vector_t<T>{};

/*
 * Testing utilities
 */
struct async_array_input_t : async_input_t
{
  explicit async_array_input_t(std::string_view src)
  : rp_(src.data())
  , ep_(src.data() + src.size())
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

template<typename Engine, typename... Args>
void run_engine_with_bufsize(
  std::size_t bufsize, Engine engine, std::string_view input, Args&&... args)
{
  async_inbuf_t inbuf(std::make_unique<async_array_input_t>(input), bufsize);
  default_scheduler_t scheduler;
  async_source_t source(inbuf, scheduler);

  engine.start(source, std::forward<Args>(args)...);

  while(auto callback = scheduler.wait())
  {
    callback();
  }
}
  
template<typename Engine, typename... Args>
void run_engine(Engine engine, std::string_view input, Args&&... args)
{
  run_engine_with_bufsize(1, engine, input, std::forward<Args>(args)...);
}
  
struct force_error_t
{
  template<typename Next, typename... Args>
  void operator()(async_source_t&, Next next, Args&&...)
  {
    next.fail("forced error");
  }
};

auto inline constexpr force_error = force_error_t{};

/*
 * Tests
 */
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

void test_read_first_digit()
{
  {
    result_t<unsigned int> result;
    auto engine = make_engine(read_first_digit, read_eof, result);
    run_engine(engine, "7");
    assert(result.value() == 7);
  }

  {
    result_t<unsigned int> result;
    auto engine = make_engine(read_first_digit, read_eof, result);
    run_engine(engine, "x");
    assert(result.error() != nullptr);
  }
}

void test_read_trailing_digits()
{
  {
    result_t<unsigned int> result;
    auto engine = make_engine(
      read_trailing_digits<unsigned int>, read_eof, result);

    unsigned int initial_total = 0U;
    unsigned int limit = 123U;
    
    run_engine(engine, "123", initial_total, limit);
    assert(result.value() == 123U);
  }

  {
    result_t<unsigned int> result;
    auto engine = make_engine(
      read_trailing_digits<unsigned int>, read_eof, result);

    unsigned int initial_total = 0U;
    unsigned int limit = 123U;
    
    run_engine(engine, "", initial_total, limit);
    assert(result.value() == 0U);
  }

  {
    result_t<unsigned int> result;
    auto engine = make_engine(
      read_trailing_digits<unsigned int>, read_eof, result);

    unsigned int initial_total = 0U;
    unsigned int limit = 100U;
    
    run_engine(engine, "123", initial_total, limit);
    assert(result.error() != nullptr);
  }

  {
    result_t<unsigned int> result;
    auto engine = make_engine(
      read_trailing_digits<unsigned int>, read_eof, result);

    unsigned int initial_total = 0U;
    unsigned int limit = 98U;
    
    run_engine(engine, "99", initial_total, limit);
    assert(result.error() != nullptr);
  }
}

void test_read_unsigned()
{
  {
    result_t<unsigned int> result;
    auto engine = make_engine(read_integral<unsigned int>, read_eof, result);
    run_engine(engine, "42");
    assert(result.value() == 42);
  }

  {
    result_t<unsigned int> result;
    auto engine = make_engine(read_integral<unsigned int>, read_eof, result);
    run_engine(engine, "-42");
    assert(result.error() != nullptr);
  }

  {
    result_t<unsigned int> result;
    auto engine = make_engine(read_integral<unsigned int>, read_eof, result);
    run_engine(engine, "\t\r 42");
    assert(result.value() == 42);
  }

  {
    result_t<unsigned int> result;
    auto engine = make_engine(read_integral<unsigned int>, read_eof, result);
    run_engine(engine, "\t\r x42");
    assert(result.error() !=  nullptr);
  }

  {
    constexpr auto ushort_limit = std::numeric_limits<unsigned short>::max();
    constexpr auto ulong_limit = std::numeric_limits<unsigned long>::max();
    if constexpr(ushort_limit < ulong_limit)
    {
      result_t<unsigned int> result;
      auto engine = make_engine(read_integral<unsigned short>,
        read_eof, result);
      auto input = std::to_string((unsigned long) ushort_limit + 1);
      run_engine(engine, std::string_view(input));
      assert(result.error() !=  nullptr);
    }
  }
}

void test_read_optional_sign()
{
  {
    result_t<bool> result;
    auto engine = make_engine(read_optional_sign, read_eof, result);
    run_engine(engine, "+");
    assert(result.value() == false);
  }

  {
    result_t<bool> result;
    auto engine = make_engine(read_optional_sign, read_eof, result);
    run_engine(engine, "-");
    assert(result.value() == true);
  }

  {
    result_t<bool> result;
    auto engine = make_engine(read_optional_sign, read_eof, result);
    run_engine(engine, "");
    assert(result.value() == false);
  }
}
    
void test_read_signed()
{
  {
    result_t<int> result;
    auto engine = make_engine(read_integral<int>, read_eof, result);
    run_engine(engine, "0");
    assert(result.value() == 0);
  }

  {
    result_t<int> result;
    auto engine = make_engine(read_integral<int>, read_eof, result);
    run_engine(engine, "-0");
    assert(result.value() == 0);
  }

  {
    result_t<int> result;
    auto engine = make_engine(read_integral<int>, read_eof, result);
    run_engine(engine, "42");
    assert(result.value() == 42);
  }

  {
    result_t<int> result;
    auto engine = make_engine(read_integral<int>, read_eof, result);
    run_engine(engine, "\t\r 42");
    assert(result.value() == 42);
  }

  {
    result_t<int> result;
    auto engine = make_engine(read_integral<int>, read_eof, result);
    run_engine(engine, "-42");
    assert(result.value() == -42);
  }

  {
    result_t<int> result;
    auto engine = make_engine(read_integral<int>, read_eof, result);
    run_engine(engine, "\t\r -42");
    assert(result.value() == -42);
  }

  {
    result_t<int> result;
    auto engine = make_engine(read_integral<int>, read_eof, result);
    run_engine(engine, "- 42");
    assert(result.error() != nullptr);
  }

  {
    result_t<int> result;
    auto engine = make_engine(read_integral<int>, read_eof, result);
    run_engine(engine, "+42");
    assert(result.value() == 42);
  }

  {
    result_t<int> result;
    auto engine = make_engine(read_integral<int>, read_eof, result);
    auto max = std::numeric_limits<int>::max();
    auto input = std::to_string(max);
    run_engine(engine, input);
    assert(result.value() == max);
  }

  {
    result_t<int> result;
    auto engine = make_engine(read_integral<int>, read_eof, result);
    unsigned int max = std::numeric_limits<int>::max();
    auto input = std::to_string(max + 1);
    run_engine(engine, input);
    assert(result.error() != nullptr);
  }

  {
    result_t<int> result;
    auto engine = make_engine(read_integral<int>, read_eof, result);
    auto max = std::numeric_limits<int>::max();
    auto input = std::to_string(max) + "0";
    run_engine(engine, input);
    assert(result.error() !=  nullptr);
  }

  {
    result_t<int> result;
    auto engine = make_engine(read_integral<int>, read_eof, result);
    auto min = std::numeric_limits<int>::min();
    auto input = std::to_string(min);
    run_engine(engine, input);
    assert(result.value() == min);
  }

  {
    result_t<int> result;
    auto engine = make_engine(read_integral<int>, read_eof, result);
    unsigned int max = std::numeric_limits<int>::max();
    auto input = "-" + std::to_string(max + 2);
    run_engine(engine, input);
    assert(result.error() != nullptr);
  }

  {
    result_t<int> result;
    auto engine = make_engine(read_integral<int>, read_eof, result);
    auto min = std::numeric_limits<int>::min();
    auto input = std::to_string(min) + "0";
    run_engine(engine, input);
    assert(result.error() != nullptr);
  }
}

void test_append_element()
{
  {
    result_t<std::vector<int>> result;
    auto engine = make_engine(append_element<int>, read_eof, result);
    run_engine(engine, "", 1, std::vector<int>());
    assert(result.value() == std::vector<int>{1});
  }
}
    
void test_read_vector()
{
  {
    result_t<std::vector<int>> result;
    auto engine = make_engine(read_vector<int>, read_eof, result);
    run_engine(engine, "[]");
    auto expected = std::vector<int>();
    assert(result.value() == expected);
  }

  {
    result_t<std::vector<int>> result;
    auto engine = make_engine(read_vector<int>, read_eof, result);
    run_engine(engine, " [ 1 2 3 ]");
    auto expected = std::vector<int>{1, 2, 3};
    assert(result.value() == expected);
  }

  {
    result_t<std::vector<int>> result;

    std::string input = "[ ";
    std::vector<int> expected;
    for(int i = 0; i != 256; ++i)
    {
      input += std::to_string(i);
      input += " ";
      expected.push_back(i);
    }
    input += "]";
      
    auto engine = make_engine(read_vector<int>, read_eof, result);
    run_engine_with_bufsize(async_inbuf_t::default_bufsize, engine, input);
    assert(result.value() == expected);
  }

  {
    result_t<std::vector<int>> result;
    auto engine = make_engine(read_vector<int>, read_eof, result);
    run_engine(engine, " [ -1 -2 -3 ]");
    auto expected = std::vector<int>{-1, -2, -3};
    assert(result.value() == expected);
  }

  {
    result_t<std::vector<int>> result;
    auto engine = make_engine(read_vector<int>, read_eof, result);
    run_engine(engine, " -1 -2 -3 ]");
    assert(result.error() != nullptr);
  }

  {
    result_t<std::vector<int>> result;
    auto engine = make_engine(read_vector<int>, read_eof, result);
    run_engine(engine, " [ -1 -2 -3");
    assert(result.error() != nullptr);
  }

  {
    result_t<std::vector<unsigned int>> result;
    auto engine = make_engine(read_vector<unsigned int>, read_eof, result);
    run_engine(engine, " [ 1 -2 3 ]");
    assert(result.error() != nullptr);
  }
}

} // anonymous

int main()
{
  test_result();
  test_read_eof();
  test_skip_spaces();
  test_read_first_digit();
  test_read_trailing_digits();
  test_read_unsigned();
  test_read_optional_sign();
  test_read_signed();
  test_append_element();
  test_read_vector();

  return 0;
}
