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

template<typename T, typename F, typename E = int>
void do_test_success(
  F&& f, std::string_view input, std::size_t bufsize, E expected = 0)
{
  async_inbuf_t inbuf(std::make_unique<async_array_input_t>(input), bufsize);
  default_scheduler_t scheduler;
  async_source_t source(inbuf, scheduler);

  async_result_t<T> result;
  f(async_result_ref(result), source);

  while(auto callback = scheduler.wait())
  {
    callback();
  }

  assert(result.available());
  assert(result.exception() == nullptr);

  if constexpr(!std::is_same_v<T, void>)
  {
    assert(result.value() == expected);
  }
  else
  {
    result.value();
  }
}

template<typename T, typename F, typename E = int>
void test_success(F&& f, std::string_view input, E expected = 0)
{
  do_test_success<T>(f, input, 1, expected);
  do_test_success<T>(f, input, async_inbuf_t::default_bufsize, expected); 
}

template<typename T, typename F>
void do_test_failure(F&& f, std::string_view input, std::size_t bufsize)
{
  async_inbuf_t inbuf(std::make_unique<async_array_input_t>(input), bufsize);
  default_scheduler_t scheduler;
  async_source_t source(inbuf, scheduler);

  async_result_t<T> result;
  f(async_result_ref(result), source);

  while(auto callback = scheduler.wait())
  {
    callback();
  }

  assert(result.available());
  assert(result.exception() != nullptr);

  bool caught = false;
  try
  {
    result.value();
  }
  catch(parse_error_t const&)
  {
    caught = true;
  }
  assert(caught);
}

template<typename T, typename F>
void test_failure(F&& f, std::string_view input)
{
  do_test_failure<T>(f, input, 1);
  do_test_failure<T>(f, input, async_inbuf_t::default_bufsize); 
}

void test_check_eom()
{
  test_success<void>(check_eom, "\n");
  test_failure<void>(check_eom, " \n");
}

} // anonymous

int main()
{
  test_check_eom();

  return 0;
}
