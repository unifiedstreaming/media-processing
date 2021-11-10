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

#include <cuti/tuple_reader.hpp>
#include <cuti/writer.hpp>

#include <cuti/bound_inbuf.hpp>
#include <cuti/bound_outbuf.hpp>
#include <cuti/cmdline_reader.hpp>
#include <cuti/default_scheduler.hpp>
#include <cuti/eof_checker.hpp>
#include <cuti/final_result.hpp>
#include <cuti/flusher.hpp>
#include <cuti/integral_readers.hpp>
#include <cuti/logger.hpp>
#include <cuti/logging_context.hpp>
#include <cuti/nb_string_inbuf.hpp>
#include <cuti/nb_string_outbuf.hpp>
#include <cuti/option_walker.hpp>
#include <cuti/quoted_string.hpp>
#include <cuti/streambuf_backend.hpp>
#include <cuti/string_reader.hpp>
#include <cuti/vector_reader.hpp>

#include <iostream>
#include <typeinfo>
#include <utility>

#undef NDEBUG
#include <cassert>

namespace // anoymous
{

using namespace cuti;

template<typename T>
void test_failing_read(logging_context_t& context,
                       std::string input,
                       std::size_t bufsize)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << '<' << typeid(T).name() <<
      ">: input: " << quoted_string(input) <<
      " bufsize: " << bufsize;
  }

  default_scheduler_t scheduler;
  stack_marker_t base_marker;

  auto inbuf = make_nb_string_inbuf(std::move(input), bufsize);
  bound_inbuf_t bit(base_marker, *inbuf, scheduler);

  final_result_t<T> read_result;
  reader_t<T> reader(read_result, bit);
  reader.start();

  unsigned int n_read_callbacks = 0;
  while(!read_result.available())
  {
    auto cb = scheduler.wait();
    assert(cb != nullptr);
    cb();
    ++n_read_callbacks;
  }

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << '<' << typeid(T).name() <<
      ">: n_read_callbacks: " << n_read_callbacks;
  }

  bool caught = false;
  try
  {
    read_result.value();
  }
  catch(std::exception const& ex)
  {
    if(auto msg = context.message_at(loglevel_t::info))
    {
      *msg << __func__ << '<' << typeid(T).name() <<
        ">: caught exception: " << ex.what();
    }
    caught = true;
  }

  assert(caught);
}

template<typename T>
void test_roundtrip(logging_context_t& context,
                    T input,
                    std::size_t bufsize)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << '<' << typeid(T).name() <<
      "> bufsize: " << bufsize;
  }

  default_scheduler_t scheduler;
  std::string serialized_form;

  stack_marker_t base_marker;

  auto outbuf = make_nb_string_outbuf(serialized_form, bufsize);
  bound_outbuf_t bot(base_marker, *outbuf, scheduler);

  final_result_t<void> write_result;
  writer_t<T> writer(write_result, bot);
  writer.start(input);

  unsigned int n_write_callbacks = 0;
  while(!write_result.available())
  {
    auto cb = scheduler.wait();
    assert(cb != nullptr);
    cb();
    ++n_write_callbacks;
  }

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << '<' << typeid(T).name() <<
      ">: n_write_callbacks: " << n_write_callbacks;
  }

  write_result.value();

  final_result_t<void> flush_result;
  flusher_t flusher(flush_result, bot);
  flusher.start();

  unsigned int n_flush_callbacks = 0;
  while(!flush_result.available())
  {
    auto cb = scheduler.wait();
    assert(cb != nullptr);
    cb();
    ++n_flush_callbacks;
  }

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << '<' << typeid(T).name() <<
      ">: n_flush_callbacks: " << n_flush_callbacks;
  }

  flush_result.value();

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << '<' << typeid(T).name() <<
      ">: serialized form: " << quoted_string(serialized_form);
  }
  
  auto inbuf = make_nb_string_inbuf(serialized_form, bufsize);
  bound_inbuf_t bit(base_marker, *inbuf, scheduler);

  final_result_t<T> read_result;
  reader_t<T> reader(read_result, bit);
  reader.start();

  unsigned int n_read_callbacks = 0;
  while(!read_result.available())
  {
    auto cb = scheduler.wait();
    assert(cb != nullptr);
    cb();
    ++n_read_callbacks;
  }

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << '<' << typeid(T).name() <<
      ">: n_read_callbacks: " << n_read_callbacks;
  }

  assert(read_result.value() == input);

  final_result_t<void> eof_result;
  eof_checker_t eof_checker(eof_result, bit);
  eof_checker.start();

  unsigned int n_eof_callbacks = 0;
  while(!eof_result.available())
  {
    auto cb = scheduler.wait();
    assert(cb != nullptr);
    cb();
    ++n_eof_callbacks;
  }

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << '<' << typeid(T).name() <<
      ">: n_eof_callbacks: " << n_eof_callbacks;
  }

  eof_result.value();
}

void test_failing_reads(logging_context_t& context, std::size_t bufsize)
{
  // missing opening curly
  test_failing_read<std::tuple<>>(context, "", bufsize);
  test_failing_read<std::tuple<>>(context, "\t\r ", bufsize);
  test_failing_read<std::pair<int, int>>(context, "", bufsize);
  test_failing_read<std::pair<int, int>>(context, "\t\r ", bufsize);

  // missing closing curly
  test_failing_read<std::tuple<>>(context, "{", bufsize);
  test_failing_read<std::tuple<>>(context, "{ \n}", bufsize);
  test_failing_read<std::tuple<int>>(context, "{ 100", bufsize);
  test_failing_read<std::tuple<int>>(context, "{ 100\n}", bufsize);
  test_failing_read<std::pair<int, int>>(context, "{ 100 101", bufsize);
  test_failing_read<std::pair<int, int>>(context, "{ 100 101\n}", bufsize);

  // error in element
  test_failing_read<std::tuple<int>>(context, "{ \"Hello world\" }", bufsize);
  test_failing_read<std::tuple<int, std::string>>(context, "{ 1 2 }", bufsize);
  test_failing_read<std::pair<int, std::string>>(context, "{ 1 2 }", bufsize);
}

auto tuple_of_tuples()
{
  return std::make_tuple(
    std::make_tuple(42, std::string("Alice")),
    std::make_tuple(66, std::string("Bob"))
  );
}

auto marx_family()
{
  using person_t = std::tuple<std::string, std::string, int>;

  auto heinrich = person_t{"Heinrich", "Marx", 1777};
  auto henriette = person_t{"Henriette", "Presburg", 1788};
  auto karl = person_t{"Karl", "Marx", 1818 };

  using family_t = std::tuple<person_t, person_t, std::vector<person_t>>;

  return family_t{heinrich, henriette, { karl }};
}

auto reverse_marx_family()
{
  using person_t = std::tuple<int, std::string, std::string>;

  auto heinrich = person_t{1777, "Heinrich", "Marx"};
  auto henriette = person_t{1788, "Henriette", "Presburg"};
  auto karl = person_t{1818, "Karl", "Marx"};

  using family_t = std::tuple<std::vector<person_t>, person_t, person_t>;

  return family_t{{karl}, heinrich, henriette};
}

void test_roundtrips(logging_context_t& context, std::size_t bufsize)
{
  test_roundtrip(context, std::tuple<>{}, bufsize);
  test_roundtrip(context, std::tuple<int>{42}, bufsize);
  test_roundtrip(context, std::tuple<int, int>{42, 4711}, bufsize);
  test_roundtrip(context, std::tuple<int, std::string>{42, "Alice"}, bufsize);
  test_roundtrip(context, tuple_of_tuples(), bufsize);
  test_roundtrip(context, marx_family(), bufsize);
  test_roundtrip(context, reverse_marx_family(), bufsize);
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

  std::size_t constexpr bufsizes[] = { 1, nb_inbuf_t::default_bufsize };
  for(auto bufsize : bufsizes)
  {
    test_failing_reads(context, bufsize);
    test_roundtrips(context, bufsize);
  }
  
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
