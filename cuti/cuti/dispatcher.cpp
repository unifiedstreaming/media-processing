/*
 * Copyright (C) 2021-2022 CodeShop B.V.
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

#include "dispatcher.hpp"

#include "default_scheduler.hpp"
#include "logging_context.hpp"
#include "tcp_connection.hpp"

#include <string>
#include <utility>
#include <vector>

namespace cuti
{

struct dispatcher_t::impl_t
{
  impl_t(logging_context_t const& logging_context,
         event_pipe_reader_t& control,
         selector_factory_t const& selector_factory)
  : logging_context_(logging_context)
  , control_(control)
  , selector_name_(selector_factory.name())
  , scheduler_(selector_factory)
  , sig_(0)
  , listeners_()
  {
    auto callback = [this] { this->on_control(); };
    control_.call_when_readable(scheduler_, callback);
  }

  impl_t(impl_t const&) = delete;
  impl_t& operator=(impl_t const&) = delete;

  void add_listener(std::unique_ptr<listener_t> listener)
  {
    listeners_.push_back(std::move(listener));
    scoped_guard_t guard([&] { listeners_.pop_back(); });

    listener_t& last = *listeners_.back();
    auto callback = [this, &last ] { this->on_listener(last); };
    last.call_when_ready(scheduler_, callback);
  
    guard.dismiss();
  }
  
  void run()
  {
    if(auto msg = logging_context_.message_at(loglevel_t::info))
    {
      *msg << "dispatcher running (selector: " << selector_name_ << ")";
    }

    sig_ = 0;
    do
    {
      auto callback = scheduler_.wait();
      assert(callback != nullptr);
      callback();
    } while(sig_ == 0);

    if(auto msg = logging_context_.message_at(loglevel_t::info))
    {
      *msg << "caught signal " << sig_ << ", stopping dispatcher";
    }
  }

private :
  void on_control()
  {
    std::optional<int> rr = control_.read();

    if(rr == std::nullopt)
    {
      // spurious callback
    }
    else if(*rr == eof)
    {
      throw system_exception_t("unexpected EOF on control pipe");
    }
    else
    {
      sig_ = *rr;
      assert(sig_ != 0);
    }

    auto callback = [this] { this->on_control(); };
    control_.call_when_readable(scheduler_, callback);
  }

  void on_listener(listener_t& listener)
  {
    std::unique_ptr<client_t> client = listener.on_ready();
    // TODO: have scheduler monitor client
    client.reset();
  
    auto callback = [this, &listener] { this->on_listener(listener); };
    listener.call_when_ready(scheduler_, callback);
  }

private :
  logging_context_t const& logging_context_;
  event_pipe_reader_t& control_;
  std::string selector_name_;
  default_scheduler_t scheduler_;
  int sig_;
  std::vector<std::unique_ptr<listener_t>> listeners_;
};

dispatcher_t::dispatcher_t(logging_context_t const& logging_context,
                           event_pipe_reader_t& control,
                           selector_factory_t const& selector_factory)
: impl_(std::make_unique<impl_t>(logging_context, control, selector_factory))
{ }

void dispatcher_t::add_listener(std::unique_ptr<listener_t> listener)
{
  impl_->add_listener(std::move(listener));
}

void dispatcher_t::run()
{
  impl_->run();
}

dispatcher_t::~dispatcher_t()
{ }

} // cuti
