/*
 * Copyright (C) 2021 CodeShop B.V.
 *
 * This file is part of the x264_encoding_service.
 *
 * The x264_encoding_service is free software: you can redistribute it
 * and/or modify it under the terms of version 2 of the GNU General
 * Public License as published by the Free Software Foundation.
 *
 * The x264_encoding_service is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See version 2 of the GNU General Public License for more details.
 *
 * You should have received a copy of version 2 of the GNU General
 * Public License along with the x264_encoding_service.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "tcp_acceptor.hpp"
#include "tcp_connection.hpp"
#include "endpoint.hpp"
#include "endpoint_list.hpp"
#include "membuf.hpp"
#include "logger.hpp"
#include "streambuf_backend.hpp"
#include "system_error.hpp"

#include <algorithm>
#include <chrono>
#include <memory>
#include <iostream>
#include <thread>

// Enable assert()
#undef NDEBUG
#include <cassert>

namespace // anonymous
{

using namespace xes;

struct reporter_t : std::ostream
{
  reporter_t(logger_t& logger, loglevel_t level)
  : std::ostream(nullptr)
  , logger_(logger)
  , level_(level)
  , buf_()
  {
    this->rdbuf(&buf_);
  }

  reporter_t(reporter_t const&) = delete;
  reporter_t& operator=(reporter_t const&) = delete;

  ~reporter_t() override
  {
    logger_.report(level_, buf_.begin(),  buf_.end());
  }
  
private :
  logger_t& logger_;
  loglevel_t level_;
  membuf_t buf_;
};

struct logging_context_t
{
  logging_context_t(logger_t& logger, loglevel_t level)
  : logger_(logger)
  , level_(level)
  { }

  std::unique_ptr<reporter_t> logging_at(loglevel_t level) const
  {
    std::unique_ptr<reporter_t> result = nullptr;
    if(level_ >= level)
    {
      result.reset(new reporter_t(logger_, level));
    }
    return result;
  }
      
private :
  logger_t& logger_;
  loglevel_t level_;
};

void blocking_accept(logging_context_t const& context,
                     endpoint_t const& endpoint)
{
  tcp_acceptor_t acceptor(endpoint);
  if(auto os = context.logging_at(loglevel_t::info))
  {
    *os << "blocking_accept: " << endpoint << " bound to " << acceptor;
  }

  tcp_connection_t client(acceptor.local_endpoint());
  if(auto os = context.logging_at(loglevel_t::info))
  {
    *os << "client side: " << client;
  }

  auto server = acceptor.accept();
  assert(server != nullptr);
  if(auto os = context.logging_at(loglevel_t::info))
  {
    *os << "server side: " << *server;
  }
}

void blocking_accept(logging_context_t const& context)
{
  endpoint_list_t endpoints(local_interfaces, any_port);
  assert(!endpoints.empty());
  
  for(auto const& endpoint : endpoints)
  {
    blocking_accept(context, endpoint);
  }
}

void nonblocking_accept(logging_context_t const& context,
                        endpoint_t const& endpoint)
{
  tcp_acceptor_t acceptor(endpoint);
  if(auto os = context.logging_at(loglevel_t::info))
  {
    *os << "nonblocking_accept: " << endpoint << " bound to " << acceptor;
  }
  acceptor.set_nonblocking();

  auto server = acceptor.accept();
  assert(server == nullptr);
  if(auto os = context.logging_at(loglevel_t::info))
  {
    *os << acceptor << " returned expected nullptr";
  }

  tcp_connection_t client(acceptor.local_endpoint());
  if(auto os = context.logging_at(loglevel_t::info))
  {
    *os << "client side: " << client;
  }

  unsigned int pause = 0;
  unsigned int attempt;
  for(attempt = 0; server == nullptr && attempt != 10; ++attempt)
  {
    if(pause != 0)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(pause));
    }
    pause = pause * 2 + 1;

    if(auto os = context.logging_at(loglevel_t::info))
    {
      *os << acceptor << ": accept(): attempt# " << attempt + 1;
    }

    server = acceptor.accept();
  }

  if(auto os = context.logging_at(loglevel_t::info))
  {
    *os << acceptor << ": " << attempt << " attempts(s)";
  }

  assert(server != nullptr);

  if(auto os = context.logging_at(loglevel_t::info))
  {
    *os << "server side: " << *server;
  }
}

void nonblocking_accept(logging_context_t const& context)
{
  endpoint_list_t endpoints(local_interfaces, any_port);
  assert(!endpoints.empty());
  
  for(auto const& endpoint : endpoints)
  {
    nonblocking_accept(context, endpoint);
  }
}

void duplicate_bind(logging_context_t const& context,
                    endpoint_t const& endpoint)
{
  tcp_acceptor_t acceptor1(endpoint);
  if(auto os = context.logging_at(loglevel_t::info))
  {
    *os << "duplicate_bind: " << endpoint << " bound to " << acceptor1;
  }

  bool caught = false;
  try
  {
    if(auto os = context.logging_at(loglevel_t::info))
    {
      *os << "binding to " << acceptor1;
    }
    tcp_acceptor_t acceptor2(acceptor1.local_endpoint());
  }
  catch(system_exception_t const& ex)
  {
    if(auto os = context.logging_at(loglevel_t::info))
    {
      *os << "caught expected exception: " << ex.what();
    }
    caught = true;
  }
  assert(caught);
}

void duplicate_bind(logging_context_t const& context)
{
  endpoint_list_t endpoints(local_interfaces, any_port);
  assert(!endpoints.empty());
  
  for(auto const& endpoint : endpoints)
  {
    duplicate_bind(context, endpoint);
  }
}

bool prove_dual_stack(logging_context_t const& context)
{
  // Get IP addresses; we expect one for IPv4, and one for IPv6.
  endpoint_list_t endpoints(local_interfaces, any_port);
  assert(std::distance(endpoints.begin(), endpoints.end()) == 2);

  // Bind to the first endpoint in the list
  auto it = endpoints.begin();
  int first_family = endpoint_family(*it);

  tcp_acceptor_t acceptor1(*it);
  if(auto os = context.logging_at(loglevel_t::info))
  {
    *os << "dual_stack: " << *it << " bound to " << acceptor1;
  }

  ++it;
  int second_family = endpoint_family(*it);
  assert(first_family != second_family);

  /*
   * Try to bind to the IP address for the second family, using the
   * port number we just obtained for the first family. There is a
   * small chance that this address is in use, so failing to bind is
   * not necessarily an error. However, if we succeed, we have proven
   * that our dual stack works.
   */
  endpoint_list_t target(ip_address(*it),
                         port_number(acceptor1.local_endpoint()));
  assert(std::distance(target.begin(), target.end()) == 1);

  bool result = false;

  try
  {
    tcp_acceptor_t acceptor2(target.front());
    if(auto os = context.logging_at(loglevel_t::info))
    {
      *os << "success binding to " << acceptor2;
    }
    result = true;
  }
  catch(system_exception_t const& ex)
  {
    if(auto os = context.logging_at(loglevel_t::info))
    {
      *os << "failed to bind to " << target.front() << ": " << ex.what();
    }
  }

  return result;
}
    
void dual_stack(logging_context_t const& context)
{
  // Check that we have multiple local interfaces (one for each family)
  endpoint_list_t endpoints(local_interfaces, any_port);
  switch(std::distance(endpoints.begin(), endpoints.end()))
  {
  case 1 :
    if(auto os = context.logging_at(loglevel_t::info))
    {
      *os << "dual_stack: single local interface - can't test";
    }
    return;
  case 2 :
    // this is what we expect
    break;
  default :
    assert(!"expected number of interfaces");
    break;
  }

  // Because of the (small) chance of a false negative, we try multiple times
  bool proven = false;
  unsigned int attempt;
  for(attempt = 0; !proven && attempt != 10; ++attempt)
  {
    proven = prove_dual_stack(context);
  }

  if(auto os = context.logging_at(loglevel_t::info))
  {
    *os << "dual_stack: " << attempt << " attempt(s)";
  }

  assert(proven);
}

int throwing_main(int argc, char const* const argv[])
{
  logger_t logger(argv[0]);
  logger.set_backend(std::make_unique<xes::streambuf_backend_t>(std::cerr));
  logging_context_t context(logger,
                            argc == 1 ? loglevel_t::error : loglevel_t::info);

  blocking_accept(context);
  nonblocking_accept(context);
  duplicate_bind(context);
  dual_stack(context);

  return 0;
}

} // anonymous

int main(int argc, char* argv[])
{
  int r = 1;

  try
  {
    r = throwing_main(argc, argv);
  }
  catch(std::exception const& ex)
  {
    std::cerr << argv[0] << ": exception: " << ex.what() << std::endl;
  }

  return r;
}
