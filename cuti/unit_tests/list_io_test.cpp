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

#include <cuti/async_writers.hpp>
#include <cuti/cmdline_reader.hpp>
#include <cuti/default_scheduler.hpp>
#include <cuti/final_result.hpp>
#include <cuti/input_list_reader.hpp>
#include <cuti/logging_context.hpp>
#include <cuti/nb_tcp_buffers.hpp>
#include <cuti/option_walker.hpp>
#include <cuti/output_list_writer.hpp>
#include <cuti/result.hpp>
#include <cuti/socket_layer.hpp>
#include <cuti/stack_marker.hpp>
#include <cuti/streambuf_backend.hpp>
#include <cuti/subroutine.hpp>
#include <cuti/tcp_connection.hpp>

#include <exception>
#include <iostream>
#include <stdexcept>
#include <utility>

#undef NDEBUG
#include <cassert>

namespace // anonymous
{

using namespace cuti;

template<typename... Values>
struct message_reader_t
{
  using result_value_t = void;

  message_reader_t(result_t<void>& result, bound_inbuf_t& buf)
  : result_(result)
  , inputs_reader_(*this, &message_reader_t::on_failure, buf)
  , eom_checker_(*this, &message_reader_t::on_failure, buf)
  , message_drainer_(*this, result_, buf)
  , ex_(nullptr)
  { }

  message_reader_t(message_reader_t const&) = delete;
  message_reader_t& operator=(message_reader_t const&) = delete;
  
  void start(stack_marker_t& base_marker, input_list_t<Values...>& inputs)
  {
    ex_ = nullptr;
    inputs_reader_.start(
      base_marker, &message_reader_t::on_inputs_read, inputs);
  }

private :
  void on_inputs_read(stack_marker_t& base_marker)
  {
    eom_checker_.start(base_marker, &message_reader_t::on_eom_checked);
  }

  void on_eom_checked(stack_marker_t& base_marker)
  {
    message_drainer_.start(base_marker, &message_reader_t::on_message_drained);
  }

  void on_failure(stack_marker_t& base_marker, std::exception_ptr ex)
  {
    assert(ex != nullptr);
    assert(ex_ == nullptr);

    ex_ = std::move(ex);
    message_drainer_.start(base_marker, &message_reader_t::on_message_drained);
  }

  void on_message_drained(stack_marker_t& base_marker)
  {
    if(ex_ != nullptr)
    {
      result_.fail(base_marker, std::move(ex_));
      return;
    }

    result_.submit(base_marker);
  }
  
private :
  result_t<void>& result_;
  subroutine_t<message_reader_t, input_list_reader_t<Values...>,
    failure_mode_t::handle_in_parent> inputs_reader_;
  subroutine_t<message_reader_t, eom_checker_t,
    failure_mode_t::handle_in_parent> eom_checker_;
  subroutine_t<message_reader_t, message_drainer_t> message_drainer_;

  std::exception_ptr ex_;
};
    
template<typename... Values>
struct message_writer_t
{
  using result_value_t = void;

  message_writer_t(result_t<void>& result, bound_outbuf_t& buf)
  : result_(result)
  , outputs_writer_(*this, &message_writer_t::on_failure, buf)
  , eom_writer_(*this, result_, buf)
  , ex_(nullptr)
  { }

  message_writer_t(message_writer_t const&) = delete;
  message_writer_t& operator=(message_writer_t const&) = delete;
  
  void start(stack_marker_t& base_marker, output_list_t<Values...>& outputs)
  {
    ex_ = nullptr;
    outputs_writer_.start(
      base_marker, &message_writer_t::on_outputs_written, outputs);
  }

private :
  void on_outputs_written(stack_marker_t& base_marker)
  {
    eom_writer_.start(base_marker, &message_writer_t::on_eom_written);
  }

  void on_failure(stack_marker_t& base_marker, std::exception_ptr ex)
  {
    assert(ex != nullptr);
    assert(ex_ == nullptr);

    ex_ = std::move(ex);
    eom_writer_.start(base_marker, &message_writer_t::on_eom_written);
  }    

  void on_eom_written(stack_marker_t& base_marker)
  {
    if(ex_ != nullptr)
    {
      result_.fail(base_marker, std::move(ex_));
      return;
    }

    result_.submit(base_marker);
  }

private :
  result_t<void>& result_;
  subroutine_t<message_writer_t, output_list_writer_t<Values...>,
    failure_mode_t::handle_in_parent> outputs_writer_;
  subroutine_t<message_writer_t, eom_writer_t> eom_writer_;
  
  std::exception_ptr ex_;
};

template<typename InputList, typename OutputList>
struct messaging_client_t;

template<typename... InputValues, typename... OutputValues>
struct messaging_client_t<input_list_t<InputValues...>,
                          output_list_t<OutputValues...>>
{
  using result_value_t = void;

  messaging_client_t(result_t<void>& result,
                     bound_inbuf_t& inbuf,
                     bound_outbuf_t& outbuf)
  : result_(result)
  , message_reader_(*this, &messaging_client_t::on_child_failure, inbuf)
  , message_writer_(*this, &messaging_client_t::on_child_failure, outbuf)
  , child_count_()
  , ex_()
  { }

  messaging_client_t(messaging_client_t const&) = delete;
  messaging_client_t& operator=(messaging_client_t const&) = delete;
  
  void start(stack_marker_t& base_marker,
             input_list_t<InputValues...>& inputs,
             output_list_t<OutputValues...>& outputs)
  {
    child_count_ = 2;
    ex_ = nullptr;

    message_reader_.start(
      base_marker, &messaging_client_t::on_child_done, inputs);
    message_writer_.start(
      base_marker, &messaging_client_t::on_child_done, outputs);
  }

private :
  void on_child_failure(stack_marker_t& base_marker, std::exception_ptr ex)
  {
    assert(ex != nullptr);

    if(ex_ == nullptr)
    {
      ex_ = std::move(ex);
    }

    this->on_child_done(base_marker);
  }

  void on_child_done(stack_marker_t& base_marker)
  {
    if(--child_count_ != 0)
    {
      return;
    }

    if(ex_ != nullptr)
    {
      result_.fail(base_marker, std::move(ex_));
      return;
    }

    result_.submit(base_marker);
  }
    
private :
  result_t<void>& result_;
  subroutine_t<messaging_client_t, message_reader_t<InputValues...>,
    failure_mode_t::handle_in_parent> message_reader_;
  subroutine_t<messaging_client_t, message_writer_t<OutputValues...>,
    failure_mode_t::handle_in_parent> message_writer_;

  int child_count_;
  std::exception_ptr ex_;
};

template<typename... InputValues, typename... OutputValues>
void perform_rpc(logging_context_t const& context,
                 input_list_t<InputValues...>& inputs,
                 nb_inbuf_t& nb_inbuf,
                 output_list_t<OutputValues...>& outputs,
                 nb_outbuf_t& nb_outbuf)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  socket_layer_t sockets;
  default_scheduler_t scheduler(sockets);

  bound_inbuf_t inbuf(nb_inbuf, scheduler);
  bound_outbuf_t outbuf(nb_outbuf, scheduler);

  final_result_t<void> result;

  messaging_client_t<
    input_list_t<InputValues...> , output_list_t<OutputValues...>>
  client(result, inbuf, outbuf);

  stack_marker_t base_marker;

  client.start(base_marker, inputs, outputs);

  unsigned int n_callbacks = 0;
  while(!result.available())
  {
    callback_t cb = scheduler.wait();
    assert(cb != nullptr);
    cb(base_marker);
    ++n_callbacks;
  }

  try
  {
    result.value();
  }
  catch(std::exception const& ex)
  {
    if(auto msg = context.message_at(loglevel_t::info))
    {
      *msg << __func__ << ": exception: " << ex.what() <<
        "; n_callbacks: " << n_callbacks;
    }
    throw;
  }

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done; n_callbacks: " << n_callbacks;
  }
}

void echo_nothing(logging_context_t const& context,
                  nb_inbuf_t& inbuf,
                  nb_outbuf_t& outbuf)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  auto inputs = make_input_list<>();
  auto outputs = make_output_list<>();

  perform_rpc(context, inputs, inbuf, outputs, outbuf);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}
  
void echo_int(logging_context_t const& context,
              nb_inbuf_t& inbuf,
              nb_outbuf_t& outbuf)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  int echoed;
  auto inputs = make_input_list<int>(echoed);

  auto outputs = make_output_list<int>(42);

  perform_rpc(context, inputs, inbuf, outputs, outbuf);

  assert(echoed == 42);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}
  
void echo_vector(logging_context_t const& context,
                 nb_inbuf_t& inbuf,
                 nb_outbuf_t& outbuf)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  std::vector<int> dst_vector;
  auto inputs = make_input_list<std::vector<int>>(dst_vector);

  std::vector<int> src_vector;
  src_vector.reserve(5000);
  for(int i = 0; i != 5000; ++i)
  {
    src_vector.push_back(i);
  }
  auto outputs = make_output_list<std::vector<int>>(src_vector);

  perform_rpc(context, inputs, inbuf, outputs, outbuf);

  assert(dst_vector == src_vector);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}
  
void echo_mixed(logging_context_t const& context,
                nb_inbuf_t& inbuf,
                nb_outbuf_t& outbuf)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  bool dst_bool = false;
  std::vector<int> dst_vector;
  std::string dst_string;

  auto inputs = make_input_list<bool, std::vector<int>, std::string>(
    dst_bool, dst_vector, dst_string);

  bool src_bool = true;

  std::vector<int> src_vector;
  src_vector.reserve(500);
  for(int i = 0; i != 500; ++i)
  {
    src_vector.push_back(i);
  }

  std::string src_string = "Principles of Programming Languages: "
    "Design, Evaluation and Implementation";
   
  auto outputs = make_output_list<bool, std::vector<int>, std::string>(
    src_bool, src_vector, src_string);

  perform_rpc(context, inputs, inbuf, outputs, outbuf);

  assert(dst_bool == src_bool);
  assert(dst_vector == src_vector);
  assert(dst_string == src_string);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}

void type_error(logging_context_t const& context,
                nb_inbuf_t& inbuf,
                nb_outbuf_t& outbuf)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  bool dst_bool = false;
  int dst_int = 0;
  std::string dst_string;

  auto inputs = make_input_list<bool, int, std::string>(
    dst_bool, dst_int, dst_string);

  bool src_bool = true;

  std::vector<int> src_vector;
  src_vector.reserve(500);
  for(int i = 0; i != 500; ++i)
  {
    src_vector.push_back(i);
  }

  std::string src_string = "Principles of Programming Languages: "
    "Design, Evaluation and Implementation";
   
  auto outputs = make_output_list<bool, std::vector<int>, std::string>(
    src_bool, src_vector, src_string);

  bool caught = false;
  try
  {
    perform_rpc(context, inputs, inbuf, outputs, outbuf);
  }
  catch(std::exception const&)
  {
    caught = true;
  }
  assert(caught);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}

void streaming_vector(logging_context_t const& context,
                      nb_inbuf_t& inbuf,
                      nb_outbuf_t& outbuf)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  std::vector<int> dst_vector;
  bool end_dst_seen = false;
  auto input_handler = [&](std::optional<int> value)
  {
    assert(!end_dst_seen);
    if(value)
    {
      dst_vector.push_back(*value);
    }
    else
    {
      end_dst_seen = true;
    }
  };

  auto inputs = make_input_list<sequence_t<int>>(input_handler);

  std::vector<int> src_vector;
  src_vector.reserve(5000);
  for(int i = 0; i != 5000; ++i)
  {
    src_vector.push_back(i);
  }

  auto output_handler =
    [ first = src_vector.begin(), last = src_vector.end() ] () mutable
  {
    return first != last ? std::make_optional(*first++) : std::nullopt;
  };

  auto outputs = make_output_list<sequence_t<int>>(output_handler);

  perform_rpc(context, inputs, inbuf, outputs, outbuf);

  assert(dst_vector == src_vector);
  assert(end_dst_seen);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}
  
void streaming_output_error(logging_context_t const& context,
                            nb_inbuf_t& inbuf,
                            nb_outbuf_t& outbuf)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  std::vector<int> dst_vector;
  bool end_dst_seen = false;
  auto input_handler = [&](std::optional<int> value)
  {
    assert(!end_dst_seen);
    if(value)
    {
      dst_vector.push_back(*value);
    }
    else
    {
      end_dst_seen = true;
    }
  };

  auto inputs = make_input_list<sequence_t<int>>(input_handler);

  std::vector<int> src_vector;
  src_vector.reserve(5000);
  for(int i = 0; i != 5000; ++i)
  {
    src_vector.push_back(i);
  }

  auto output_handler =
    [ first = src_vector.begin(), last = src_vector.end() ] () mutable
  {
    if(last - first == 1000)
    {
      throw std::runtime_error("output handler failure");
    }
    return first != last ? std::make_optional(*first++) : std::nullopt;
  };

  auto outputs = make_output_list<sequence_t<int>>(output_handler);

  bool caught = false;
  try
  {
    perform_rpc(context, inputs, inbuf, outputs, outbuf);
  }
  catch(std::exception const&)
  {
    caught = true;
  }
  assert(caught);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}
  
void streaming_input_error(logging_context_t const& context,
                           nb_inbuf_t& inbuf,
                           nb_outbuf_t& outbuf)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  std::vector<int> dst_vector;
  bool end_dst_seen = false;
  auto input_handler = [&](std::optional<int> value)
  {
    if(dst_vector.size() == 4000)
    {
      throw std::runtime_error("input handler failure");
    }
      
    assert(!end_dst_seen);
    if(value)
    {
      dst_vector.push_back(*value);
    }
    else
    {
      end_dst_seen = true;
    }
  };

  auto inputs = make_input_list<sequence_t<int>>(input_handler);

  std::vector<int> src_vector;
  src_vector.reserve(5000);
  for(int i = 0; i != 5000; ++i)
  {
    src_vector.push_back(i);
  }

  auto output_handler =
    [ first = src_vector.begin(), last = src_vector.end() ] () mutable
  {
    return first != last ? std::make_optional(*first++) : std::nullopt;
  };

  auto outputs = make_output_list<sequence_t<int>>(output_handler);

  bool caught = false;
  try
  {
    perform_rpc(context, inputs, inbuf, outputs, outbuf);
  }
  catch(std::exception const&)
  {
    caught = true;
  }
  assert(caught);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}
  
void double_streaming_error(logging_context_t const& context,
                            nb_inbuf_t& inbuf,
                            nb_outbuf_t& outbuf)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting";
  }

  std::vector<int> dst_vector;
  bool end_dst_seen = false;
  auto input_handler = [&](std::optional<int> value)
  {
    if(dst_vector.size() == 2500)
    {
      throw std::runtime_error("input handler failure");
    }
      
    assert(!end_dst_seen);
    if(value)
    {
      dst_vector.push_back(*value);
    }
    else
    {
      end_dst_seen = true;
    }
  };

  auto inputs = make_input_list<sequence_t<int>>(input_handler);

  std::vector<int> src_vector;
  src_vector.reserve(5000);
  for(int i = 0; i != 5000; ++i)
  {
    src_vector.push_back(i);
  }

  auto output_handler =
    [ first = src_vector.begin(), last = src_vector.end() ] () mutable
  {
    if(last - first == 1000)
    {
      throw std::runtime_error("output handler failure");
    }
    return first != last ? std::make_optional(*first++) : std::nullopt;
  };

  auto outputs = make_output_list<sequence_t<int>>(output_handler);

  bool caught = false;
  try
  {
    perform_rpc(context, inputs, inbuf, outputs, outbuf);
  }
  catch(std::exception const&)
  {
    caught = true;
  }
  assert(caught);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
  }
}
  
void do_run_tests(logging_context_t const& context, std::size_t bufsize)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": starting; bufsize: " << bufsize;
  }

  socket_layer_t sockets;

  std::unique_ptr<tcp_connection_t> this_end;
  std::unique_ptr<tcp_connection_t> that_end;
  std::tie(this_end, that_end) = make_connected_pair(sockets);

  std::unique_ptr<nb_inbuf_t> this_in;
  std::unique_ptr<nb_outbuf_t> this_out;
  std::tie(this_in, this_out) =
    make_nb_tcp_buffers(std::move(this_end), bufsize, bufsize);
  
  std::unique_ptr<nb_inbuf_t> that_in;
  std::unique_ptr<nb_outbuf_t> that_out;
  std::tie(that_in, that_out) =
    make_nb_tcp_buffers(std::move(that_end), bufsize, bufsize);
  
  echo_nothing(context, *this_in, *that_out);
  type_error(context, *this_in, *that_out);
  echo_int(context, *this_in, *that_out);
  streaming_output_error(context, *this_in, *that_out);
  echo_vector(context, *this_in, *that_out);
  streaming_input_error(context, *this_in, *that_out);
  echo_mixed(context, *this_in, *that_out);
  double_streaming_error(context, *this_in, *that_out);
  streaming_vector(context, *this_in, *that_out);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << __func__ << ": done";
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

  std::size_t constexpr bufsizes[] =
    { 1, nb_inbuf_t::default_bufsize };
  for(auto bufsize : bufsizes)
  {
    do_run_tests(context, bufsize);
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
