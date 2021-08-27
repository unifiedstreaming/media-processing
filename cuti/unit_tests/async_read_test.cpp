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

#include <cuti/async_read.hpp>

#include <cuti/async_inbuf.hpp>
#include <cuti/async_input.hpp>
#include <cuti/async_result.hpp>
#include <cuti/async_stitch.hpp>
#include <cuti/default_scheduler.hpp>
#include <cuti/parse_error.hpp>
#include <cuti/ticket_holder.hpp>

#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>

// Enable assert()
#undef NDEBUG
#include <cassert>

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

/*
 * Multiplies the trailing decimal part of a string value by 10.
 */
std::string decimals_times_ten(std::string const& input)
{
  return input + '0';
}

/*
 * Adds one to the trailing decimal part of a string value.  In other
 * words: if the string value is negative, it subtracts 1.
 */
std::string decimals_plus_one(std::string const& input)
{
  std::string result;
 
  bool carry = true;
  auto pos = input.rbegin();
  while(pos != input.rend() && carry && *pos >= '0' && *pos <= '9')
  {
    if(*pos < '9')
    {
      result += *pos + 1;
      carry = false;
    }
    else
    {
      result += '0';
    }
    ++pos;
  }

  if(carry)
  {
    result += '1';
  }

  while(pos != input.rend())
  {
    result += *pos;
    ++pos;
  }

  std::reverse(result.begin(), result.end());
  return result;
}
  
template<typename T, typename F>
void do_test_value_success(F f, std::string_view input, std::size_t bufsize,
                           T const& expected)
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

  assert(result.value() == expected);
}

template<typename T, typename F>
void do_test_value_failure(F f, std::string_view input, std::size_t bufsize)
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
void do_test_void_success(F f, std::string_view input, std::size_t bufsize)
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

  result.value();
}

template<typename F>
void do_test_void_failure(F f, std::string_view input, std::size_t bufsize)
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
void test_value_success(F f, std::string_view input, T const& expected)
{
  do_test_value_success(f, input, 1, expected);
  do_test_value_success(f, input, async_inbuf_t::default_bufsize, expected);
}

template<typename T, typename F>
void test_value_failure(F f, std::string_view input)
{
  do_test_value_failure<T>(f, input, 1);
  do_test_value_failure<T>(f, input, async_inbuf_t::default_bufsize);
}

template<typename F>
void test_void_success(F f, std::string_view input)
{
  do_test_void_success(f, input, 1);
  do_test_void_success(f, input, async_inbuf_t::default_bufsize);
}

template<typename F>
void test_void_failure(F f, std::string_view input)
{
  do_test_void_failure(f, input, 1);
  do_test_void_failure(f, input, async_inbuf_t::default_bufsize);
}

void test_decimals_times_ten()
{
  assert(decimals_times_ten("1") == "10");
  assert(decimals_times_ten("99") == "990");
  assert(decimals_times_ten("-1") == "-10");
  assert(decimals_times_ten("-99") == "-990");
}

void test_decimals_plus_one()
{
  assert(decimals_plus_one("1") == "2");
  assert(decimals_plus_one("99") == "100");
  assert(decimals_plus_one("-1") == "-2");
  assert(decimals_plus_one("-99") == "-100");
}
  
void test_drop_source()
{
  test_void_success(drop_source, "");
}

void test_read_eof()
{
  auto chain = async_stitch(read_eof, drop_source);
  
  test_void_success(chain, "");
  test_void_failure(chain, " ");
}

void test_skip_whitespace()
{
  auto chain = async_stitch(detail::skip_whitespace, read_eof, drop_source);
  
  test_void_success(chain, "");
  test_void_success(chain, "\t\r ");
  test_void_failure(chain, "x");
}

void test_read_bool()
{
  auto chain = async_stitch(async_read<bool>, read_eof, drop_source);

  test_value_success<bool>(chain, "~", false);
  test_value_success<bool>(chain, "\t\r ~", false);
  test_value_success<bool>(chain, "*", true);
  test_value_success<bool>(chain, "\t\r *", true);

  test_value_failure<bool>(chain, "");
  test_value_failure<bool>(chain, "\t\r ");
  test_value_failure<bool>(chain, "x");
  test_value_failure<bool>(chain, "\t\r x");
}
  
void test_read_first_digit()
{
  auto chain = async_stitch(detail::read_first_digit<unsigned int>,
    read_eof, drop_source);

  test_value_success<unsigned int>(chain, "0", 0);
  test_value_success<unsigned int>(chain, "9", 9);

  test_value_failure<unsigned int>(chain, "/");
  test_value_failure<unsigned int>(chain, "/");
  test_value_failure<unsigned int>(chain, "");
}
  
template<typename T>
void do_test_read_unsigned()
{
  auto chain = async_stitch(async_read<T>, read_eof, drop_source);
  auto max = std::numeric_limits<T>::max();

  test_value_success<T>(chain, "0", T(0));
  test_value_success<T>(chain, "\t\r 0", T(0));
  test_value_success<T>(chain, std::to_string(max), max);

  test_value_failure<T>(chain, "x");
  test_value_failure<T>(chain, decimals_times_ten(std::to_string(max)));
  test_value_failure<T>(chain, decimals_plus_one(std::to_string(max)));
}
  
void test_read_unsigned()
{
  do_test_read_unsigned<unsigned short>();
  do_test_read_unsigned<unsigned int>();
  do_test_read_unsigned<unsigned long>();
  do_test_read_unsigned<unsigned long long>();
}

void test_read_optional_sign()
{
  auto chain = async_stitch(detail::read_optional_sign,
    read_eof, drop_source);

  test_value_success<detail::sign_t>(chain, "-", detail::sign_t::negative);
  test_value_success<detail::sign_t>(chain, "+", detail::sign_t::positive);
  test_value_success<detail::sign_t>(chain, "", detail::sign_t::positive);
}
  
template<typename T>
void do_test_read_signed()
{
  auto chain = async_stitch(async_read<T>, read_eof, drop_source);
  auto min = std::numeric_limits<T>::min();
  auto max = std::numeric_limits<T>::max();

  test_value_success<T>(chain, std::to_string(min), min);
  test_value_success<T>(chain, "\t\r 0", T(0));
  test_value_success<T>(chain, "\t\r -0", T(0));
  test_value_success<T>(chain, "\t\r +0", T(0));
  test_value_success<T>(chain, std::to_string(max), max);

  test_value_failure<T>(chain, "x");
  test_value_failure<T>(chain, decimals_times_ten(std::to_string(min)));
  test_value_failure<T>(chain, decimals_plus_one(std::to_string(min)));
  test_value_failure<T>(chain, decimals_times_ten(std::to_string(max)));
  test_value_failure<T>(chain, decimals_plus_one(std::to_string(max)));
}
  
void test_read_signed()
{
  do_test_read_unsigned<short>();
  do_test_read_unsigned<int>();
  do_test_read_unsigned<long>();
  do_test_read_unsigned<long long>();
}

void test_read_double_quote()
{
  auto chain = async_stitch(
    detail::read_double_quote, read_eof, drop_source);

  test_void_success(chain, "\"");
  test_void_failure(chain, "\n");
  test_void_failure(chain, "");
}

void test_read_string()
{
  auto chain = async_stitch(
    async_read<std::string>, read_eof, drop_source);

  test_value_success<std::string>(chain, "\"\"", "");
  test_value_success<std::string>(chain, "\t\r \"\"", "");
  test_value_success<std::string>(chain, "\"hello world\"", "hello world");
  test_value_success<std::string>(chain, "\"\\t\\n\\r\\\\\\\"\"",
    "\t\n\r\\\"");

  {
    std::string expected;
    expected += '\0';
    test_value_success<std::string>(chain, "\"\\0\"", expected);
  }

  {
    std::string expected;
    expected += '\0';
    test_value_success<std::string>(chain, "\"\\x00\"", expected);
  }

  test_value_success<std::string>(chain,
    "\"\\x01\\x23\\x45\\x67\\x89\\xAB\\xCD\\xEF\\xab\\xcd\\xef\"",
    "\x01\x23\x45\x67\x89\xAB\xCD\xEF\xAB\xCD\xEF");

  test_value_failure<std::string>(chain, "");
  test_value_failure<std::string>(chain, "\"");
  test_value_failure<std::string>(chain, "\"\n\"");
  test_value_failure<std::string>(chain, "\"\t\"");
  test_value_failure<std::string>(chain, "\"\\x\"");
  test_value_failure<std::string>(chain, "\"\\xg\"");
  test_value_failure<std::string>(chain, "\"\\xa\"");
  test_value_failure<std::string>(chain, "\"\\xag\"");
}

void test_read_begin_sequence()
{
  auto chain = async_stitch(
    detail::read_begin_sequence, read_eof, drop_source);

  test_void_success(chain, "[");
  test_void_failure(chain, "]");
}

void test_read_sequence()
{
  {
    auto chain = async_stitch(
      async_read<std::vector<int>>, read_eof, drop_source);

    test_value_success<std::vector<int>>(
      chain, "[]", std::vector<int>{});
    test_value_success<std::vector<int>>(
      chain, " [ ]", std::vector<int>{});
    test_value_success<std::vector<int>>(
      chain, " [ 42 ]", std::vector<int>{42});
    test_value_success<std::vector<int>>(
      chain, " [ 1 -2 3 ]", std::vector<int>{1, -2, 3});

    {
      std::string input = "[";
      std::vector<int> expected;
      for(int i = 0; i != 250; ++i)
      {
        input += ' ';
        input += std::to_string(i);
        expected.push_back(i);
      }
      input += " ]";

      test_value_success<std::vector<int>>(chain, input, expected);
    }

    test_value_failure<std::vector<int>>(chain, " x");
    test_value_failure<std::vector<int>>(chain, "[ 42");
  }

  {
    auto chain = async_stitch(
      async_read<std::vector<std::vector<int>>>, read_eof, drop_source);

    std::string input = "[";
    std::vector<std::vector<int>> expected;
    for(int i = 0; i != 100; ++i)
    {
      std::string subinput = "[";
      std::vector<int> subexpected;
      for(int j = 0; j != 100; ++j)
      {
        subinput += ' ';
        subinput += std::to_string(j);
        subexpected.push_back(j);
      }
      subinput += " ]";
      input += ' ';
      input += subinput;
      expected.push_back(std::move(subexpected));
    }
    input += " ]";
        
    test_value_success<std::vector<std::vector<int>>>(chain, input, expected);
  }
}

struct person_t
{
  static std::string validate_name(std::string name)
  {
    if(name.empty())
    {
      throw std::runtime_error("name is empty");
    }
    return name;
  }

  person_t(std::string first_name, std::string last_name, int year_of_birth)
  : first_name_(validate_name(std::move(first_name)))
  , last_name_(validate_name(std::move(last_name)))
  , year_of_birth_(year_of_birth)
  { }
  
  std::string first_name_;
  std::string last_name_;
  int year_of_birth_;
};

person_t make_reversed_person(int year_of_birth,
                              std::string last_name,
                              std::string first_name)
{
  return person_t(std::move(first_name),
                  std::move(last_name),
                  year_of_birth);
}
     
bool operator==(person_t const& lhs, person_t const& rhs)
{
  return
    lhs.first_name_ == rhs.first_name_ &&
    lhs.last_name_ == rhs.last_name_ &&
    lhs.year_of_birth_ == rhs.year_of_birth_;
}

struct family_t
{
  person_t father_;
  person_t mother_;
  std::vector<person_t> children_;
};
  
bool operator==(family_t const& lhs, family_t const& rhs)
{
  return
    lhs.father_ == rhs.father_ &&
    lhs.mother_ == rhs.mother_ &&
    lhs.children_ == rhs.children_;
}

} // anonymous

template<>
inline auto constexpr cuti::async_read<person_t> =
  cuti::async_construct<person_t, std::string, std::string, int>;

template<>
inline auto constexpr cuti::async_read<family_t> =
  cuti::async_construct<family_t, person_t, person_t, std::vector<person_t>>;

namespace // anonymous
{

void test_read_struct()
{
  person_t const heinrich{"Heinrich", "Marx", 1777};
  person_t const henriette{"Henriette", "Presburg", 1788};
  person_t const karl{"Karl", "Marx", 1818};

  {
    static auto constexpr chain =
      async_stitch(async_read<person_t>, read_eof, drop_source);

    test_value_success<person_t>(chain, " { \"Karl\" \"Marx\" 1818 }", karl);
    test_value_failure<person_t>(chain, " \"Karl\" \"Marx\" 1818 }");
    test_value_failure<person_t>(chain, " { \"Karl\" \"Marx\" 1818");
    test_value_failure<person_t>(chain, " { \"Karl\" \"Marx\" \"1818\" }");
    test_value_failure<person_t>(chain, " { \"Karl\" \"Marx\" }");
    test_value_failure<person_t>(chain, " { \"\" \"Marx\" 1818 }");
  }

  {
    static auto constexpr chain = async_stitch(
      make_async_builder<
        int, std::string, std::string>(make_reversed_person),
      read_eof, drop_source);

    test_value_success<person_t>(chain, " { 1818  \"Marx\" \"Karl\" }", karl);
  }

  {
    std::vector<person_t> const folks{heinrich, henriette, karl};

    static auto constexpr chain =
      async_stitch(async_read<std::vector<person_t>>, read_eof, drop_source);

    test_value_success<std::vector<person_t>>(
      chain,
      "[{ \"Heinrich\" \"Marx\" 1777 }"
        "{ \"Henriette\" \"Presburg\" 1788 }"
        "{ \"Karl\" \"Marx\" 1818 }]",
      folks);
  }

  {
    family_t const family{heinrich, henriette, {karl}};

    static auto constexpr chain =
      async_stitch(async_read<family_t>, read_eof, drop_source);

    test_value_success<family_t>(
      chain,
      "{{ \"Heinrich\" \"Marx\" 1777 }"
        "{ \"Henriette\" \"Presburg\" 1788 }"
        "[{ \"Karl\" \"Marx\" 1818 }]}",
      family);
  }
}

} // anonymous

int main()
{
  test_decimals_times_ten();
  test_decimals_plus_one();

  test_drop_source();
  test_read_eof();
  test_skip_whitespace();

  test_read_bool();
  test_read_first_digit();
  test_read_unsigned();
  test_read_optional_sign();
  test_read_signed();
  test_read_begin_sequence();
  test_read_double_quote();
  test_read_string();
  test_read_sequence();
  test_read_struct();

  return 0;
}
