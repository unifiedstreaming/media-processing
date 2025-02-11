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

#ifndef CUTI_IO_TEST_UTILS_HPP_
#define CUTI_IO_TEST_UTILS_HPP_

#include "async_readers.hpp"
#include "async_writers.hpp"
#include "bound_inbuf.hpp"
#include "bound_outbuf.hpp"
#include "default_scheduler.hpp"
#include "eof_reader.hpp"
#include "final_result.hpp"
#include "flusher.hpp"
#include "loglevel.hpp"
#include "logging_context.hpp"
#include "nb_string_inbuf.hpp"
#include "nb_string_outbuf.hpp"
#include "quoted.hpp"
#include "socket_layer.hpp"
#include "stack_marker.hpp"

#include <cstddef>
#include <functional>
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

namespace // anonymous
{

template<typename T>
void test_failing_read(logging_context_t const& context,
                       std::size_t bufsize,
                       std::string input)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << '<' << typeid(T).name() <<
      ">: starting; bufsize: " << bufsize <<
      " input: " << quoted_string(input);
  }

  socket_layer_t sockets;
  default_scheduler_t scheduler(sockets);

  auto inbuf = make_nb_string_inbuf(std::move(input), bufsize);
  bound_inbuf_t bit(*inbuf, scheduler);

  stack_marker_t base_marker;

  final_result_t<T> read_result;
  reader_t<T> reader(read_result, bit);
  reader.start(base_marker);

  std::size_t n_reading_callbacks = 0;
  while(!read_result.available())
  {
    auto cb = scheduler.wait();
    assert(cb != nullptr);
    cb(base_marker);
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

template<typename T, typename Eq>
void test_roundtrip(logging_context_t const& context,
                    std::size_t bufsize,
                    T value,
                    Eq eq)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << '<' << typeid(T).name() <<
      ">: starting; bufsize: " << bufsize;
  }

  socket_layer_t sockets;
  default_scheduler_t scheduler(sockets);

  std::string serialized_form;
  auto outbuf = make_nb_string_outbuf(serialized_form, bufsize);
  bound_outbuf_t bot(*outbuf, scheduler);

  stack_marker_t base_marker;

  final_result_t<void> write_result;
  writer_t<T> writer(write_result, bot);
  writer.start(base_marker, value);

  std::size_t n_writing_callbacks = 0;
  while(!write_result.available())
  {
    auto cb = scheduler.wait();
    assert(cb != nullptr);
    cb(base_marker);
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
  flusher.start(base_marker);

  std::size_t n_flushing_callbacks = 0;
  while(!flush_result.available())
  {
    auto cb = scheduler.wait();
    assert(cb != nullptr);
    cb(base_marker);
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
      ">: serialized form (size: " << size << ')';

    if(size <= 256)
    {
      *msg << ": " << quoted_string(serialized_form);
    }
    else
    {
      std::string truncated(serialized_form.begin(),
                            serialized_form.begin() + 256);
      *msg << " <truncated>: " << quoted_string(truncated);
    }
  }

  auto inbuf = make_nb_string_inbuf(std::move(serialized_form), bufsize);

  bound_inbuf_t bit(*inbuf, scheduler);

  final_result_t<T> read_result;
  reader_t<T> reader(read_result, bit);
  reader.start(base_marker);

  std::size_t n_reading_callbacks = 0;
  while(!read_result.available())
  {
    auto cb = scheduler.wait();
    assert(cb != nullptr);
    cb(base_marker);
    ++n_reading_callbacks;
  }

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << '<' << typeid(T).name() <<
      ">: n_reading_callbacks: " << n_reading_callbacks;
  }

  assert(eq(read_result.value(), value));

  final_result_t<void> eof_reader_result;
  eof_reader_t eof_reader(eof_reader_result, bit);
  eof_reader.start(base_marker);

  std::size_t n_eof_reader_callbacks = 0;
  while(!eof_reader_result.available())
  {
    auto cb = scheduler.wait();
    assert(cb != nullptr);
    cb(base_marker);
    ++n_eof_reader_callbacks;
  }

  eof_reader_result.value();

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << '<' << typeid(T).name() <<
      ">: n_eof_reader_callbacks: " << n_eof_reader_callbacks;
  }
}

template<typename T>
void test_roundtrip(logging_context_t const& context,
                    std::size_t bufsize,
                    T value)
{
  test_roundtrip(context, bufsize, value, std::equal_to<T>{});
}

} // anonymous

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
