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
#include <cuti/async_input.hpp>
#include <cuti/async_result.hpp>
#include <cuti/async_serializers.hpp>
#include <cuti/async_stitch.hpp>
#include <cuti/default_scheduler.hpp>
#include <cuti/parse_error.hpp>
#include <cuti/ticket_holder.hpp>

#include <string_view>
#include <type_traits>

// Enable assert()
#undef NDEBUG

namespace // anonymous
{

using namespace cuti;

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

template<typename T, typename F>
void do_test_value_success(F&& f, std::string_view input, std::size_t bufsize,
                           T expected)
{
  async_inbuf_t inbuf(std::make_unique<async_array_input_t>(input), bufsize);
  default_scheduler_t scheduler;
  async_source_t source(inbuf, scheduler);

  async_result_t<T> result;
  f(async_result_ref(result), source);

  while(!result.available())
  {
    auto callback = scheduler.wait();
    assert(callback != nullptr);
    callback();
  }

  assert(result.exception() == nullptr);
  assert(result.value() == expected);
}

template<typename T, typename F>
void do_test_value_failure(F&& f, std::string_view input, std::size_t bufsize)
{
  async_inbuf_t inbuf(std::make_unique<async_array_input_t>(input), bufsize);
  default_scheduler_t scheduler;
  async_source_t source(inbuf, scheduler);

  async_result_t<T> result;
  f(async_result_ref(result), source);

  while(!result.available())
  {
    auto callback = scheduler.wait();
    assert(callback != nullptr);
    callback();
  }
  
  assert(result.exception() != nullptr);
  bool caught = false;
  try
  {
    result.value();
  }
  catch(std::exception const&)
  {
    caught = true;
  }
  assert(caught);
}

template<typename F>
void do_test_void_success(F&& f, std::string_view input, std::size_t bufsize)
{
  async_inbuf_t inbuf(std::make_unique<async_array_input_t>(input), bufsize);
  default_scheduler_t scheduler;
  async_source_t source(inbuf, scheduler);

  async_result_t<void> result;
  f(async_result_ref(result), source);

  while(!result.available())
  {
    auto callback = scheduler.wait();
    assert(callback != nullptr);
    callback();
  }

  assert(result.exception() == nullptr);
  result.value();
}

template<typename F>
void do_test_void_failure(F&& f, std::string_view input, std::size_t bufsize)
{
  async_inbuf_t inbuf(std::make_unique<async_array_input_t>(input), bufsize);
  default_scheduler_t scheduler;
  async_source_t source(inbuf, scheduler);

  async_result_t<void> result;
  f(async_result_ref(result), source);

  while(!result.available())
  {
    auto callback = scheduler.wait();
    assert(callback != nullptr);
    callback();
  }
  
  assert(result.exception() != nullptr);
  bool caught = false;
  try
  {
    result.value();
  }
  catch(std::exception const&)
  {
    caught = true;
  }
  assert(caught);
}

template<typename T, typename F>
void test_value_success(F&& f, std::string_view input, T expected)
{
  do_test_value_success(f, input, 1, expected);
  do_test_value_success(f, input, async_inbuf_t::default_bufsize, expected);
}

template<typename T, typename F>
void test_value_failure(F&& f, std::string_view input)
{
  do_test_value_failure<T>(f, input, 1);
  do_test_value_failure<T>(f, input, async_inbuf_t::default_bufsize);
}

template<typename F>
void test_void_success(F&& f, std::string_view input)
{
  do_test_void_success(f, input, 1);
  do_test_void_success(f, input, async_inbuf_t::default_bufsize);
}

template<typename F>
void test_void_failure(F&& f, std::string_view input)
{
  do_test_void_failure(f, input, 1);
  do_test_void_failure(f, input, async_inbuf_t::default_bufsize);
}

void test_drop_source()
{
  test_void_success(drop_source, "");
}

void test_check_eof()
{
  auto chain = async_stitch(check_eof, drop_source);
  
  test_void_success(chain, "");
  test_void_failure(chain, " ");
}

void test_skip_whitespace()
{
  auto chain = async_stitch(skip_whitespace, check_eof, drop_source);
  
  test_void_success(chain, "");
  test_void_success(chain, "\t\r ");
  test_void_failure(chain, "x");
}

void test_read_first_digit()
{
  auto chain = async_stitch(read_first_digit, check_eof, drop_source);

  test_value_success(chain, "0", 0);
  test_value_success(chain, "9", 9);

  test_value_failure<int>(chain, "/");
  test_value_failure<int>(chain, "/");
  test_value_failure<int>(chain, "");
}
  
} // anonymous

int main()
{
  test_drop_source();
  test_check_eof();
  test_skip_whitespace();
  test_read_first_digit();

  return 0;
}
