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

#ifndef CUTI_IO_TEST_UTILS_HPP_
#define CUTI_IO_TEST_UTILS_HPP_

#include <cuti/bound_inbuf.hpp>
#include <cuti/bound_outbuf.hpp>
#include <cuti/default_scheduler.hpp>
#include <cuti/eof_checker.hpp>
#include <cuti/final_result.hpp>
#include <cuti/flusher.hpp>
#include <cuti/loglevel.hpp>
#include <cuti/logging_context.hpp>
#include <cuti/nb_string_inbuf.hpp>
#include <cuti/nb_string_outbuf.hpp>
#include <cuti/quoted_string.hpp>
#include <cuti/reader_traits.hpp>
#include <cuti/stack_marker.hpp>
#include <cuti/reader_traits.hpp>
#include <cuti/writer_traits.hpp>

#include <cstddef>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <utility>

/*
 * Enable assert(), remembering its old setting
 */
#ifdef NDEBUG
#undef NDEBUG
#define CUTI_IO_TEST_UTILS_RESTORE_NDEBUG_
#else
#undef CUTI_IO_TEST_UTILS_RESTORE_NDEBUG_
#endif
#include <cassert>

namespace cuti
{

namespace io_test_utils
{

template<typename T>
void test_failing_read(logging_context_t& context, std::size_t bufsize,
                       std::string input)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << '<' << typeid(T).name() <<
      ">: starting; bufsize: " << bufsize <<
      " input: " << quoted_string(input);
  }

  default_scheduler_t scheduler;
  stack_marker_t marker;

  auto inbuf = make_nb_string_inbuf(std::move(input), bufsize);
  bound_inbuf_t bit(marker, *inbuf, scheduler);

  final_result_t<T> read_result;
  reader_t<T> reader(read_result, bit);
  reader.start();

  std::size_t n_reading_callbacks = 0;
  while(!read_result.available())
  {
    auto cb = scheduler.wait();
    assert(cb != nullptr);
    cb();
    ++n_reading_callbacks;
  }

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << '<' << typeid(T).name() <<
      ">: n_reading_callbacks: " << n_reading_callbacks;
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
        ">: caught required exception: " << ex.what();
    }
    caught = true;
  }

  assert(caught);
}

template<typename T, typename... WriterArgs>
void test_roundtrip(logging_context_t& context,
                    std::size_t bufsize, T value,
                    WriterArgs&&... writer_args)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << '<' << typeid(T).name() <<
      ">: starting; bufsize: " << bufsize;
  }

  default_scheduler_t scheduler;
  stack_marker_t marker;

  std::string serialized_form;
  auto outbuf = make_nb_string_outbuf(serialized_form, bufsize);
  bound_outbuf_t bot(marker, *outbuf, scheduler);

  final_result_t<void> write_result;
  writer_t<T> writer(write_result, bot,
                     std::forward<WriterArgs>(writer_args)...);
  writer.start(value);

  std::size_t n_writing_callbacks = 0;
  while(!write_result.available())
  {
    auto cb = scheduler.wait();
    assert(cb != nullptr);
    cb();
    ++n_writing_callbacks;
  }

  write_result.value();

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << '<' << typeid(T).name() <<
      ">: n_writing_callbacks: " << n_writing_callbacks;
  }

  final_result_t<void> flush_result;
  flusher_t flusher(flush_result, bot);
  flusher.start();

  std::size_t n_flushing_callbacks = 0;
  while(!flush_result.available())
  {
    auto cb = scheduler.wait();
    assert(cb != nullptr);
    cb();
    ++n_flushing_callbacks;
  }

  flush_result.value();

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << '<' << typeid(T).name() <<
      ">: n_flushing_callbacks: " << n_flushing_callbacks;
  }

  if(auto msg = context.message_at(loglevel_t::info))
  {
    auto size = serialized_form.size();

    *msg << __func__ << '<' << typeid(T).name() <<
      ">: serialized form (size: " << size << ") ";

    if(size <= 256)
    {
      *msg << ':' << quoted_string(serialized_form);
    }
    else
    {
      std::string truncated(serialized_form.begin(),
                            serialized_form.begin() + 256);
      *msg << "<truncated>: " << quoted_string(truncated);
    }
  }

  auto inbuf = make_nb_string_inbuf(std::move(serialized_form), bufsize);

  bound_inbuf_t bit(marker, *inbuf, scheduler);

  final_result_t<T> read_result;
  reader_t<T> reader(read_result, bit);
  reader.start();

  std::size_t n_reading_callbacks = 0;
  while(!read_result.available())
  {
    auto cb = scheduler.wait();
    assert(cb != nullptr);
    cb();
    ++n_reading_callbacks;
  }

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << '<' << typeid(T).name() <<
      ">: n_reading_callbacks: " << n_reading_callbacks;
  }

  assert(read_result.value() == value);

  final_result_t<void> checker_result;
  eof_checker_t checker(checker_result, bit);
  checker.start();

  std::size_t n_checking_callbacks = 0;
  while(!checker_result.available())
  {
    auto cb = scheduler.wait();
    assert(cb != nullptr);
    cb();
    ++n_checking_callbacks;
  }

  checker_result.value();

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << '<' << typeid(T).name() <<
      ">: n_checking_callbacks: " << n_checking_callbacks;
  }
}

} // io_test_utils

} // cuti

/*
 * Restore original assert() behavior
 */
#ifdef CUTI_IO_TEST_UTILS_RESTORE_NDEBUG_
#undef CUTI_IO_TEST_UTILS_RESTORE_NDEBUG_
#define NDEBUG
#endif
#include <cassert>

#endif
