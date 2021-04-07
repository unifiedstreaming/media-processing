/*
 * Copyright (C) 2021 CodeShop B.V.
 *
 * This file is part of the cuti library.
 *
 * The cuti library is free software: you can redistribute it
 * and/or modify it under the terms of version 2 of the GNU General
 * Public License as published by the Free Software Foundation.
 *
 * The cuti library is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See version 2 of the GNU General Public License for more details.
 *
 * You should have received a copy of version 2 of the GNU General
 * Public License along with the cuti library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "logger.hpp"

#include "exception_builder.hpp"
#include "format.hpp"
#include "membuf.hpp"
#include "default_backend.hpp"
#include "logging_backend.hpp"
#include "system_error.hpp"

#include <cstring>
#include <limits>
#include <stdexcept>
#include <utility>

namespace cuti
{

char const* loglevel_string(loglevel_t level)
{
  switch(level)
  {
  case loglevel_t::error : return "error";
  case loglevel_t::warning : return "warning";
  case loglevel_t::info: return "info";
  case loglevel_t::debug: return "debug";
  }

  return "<invalid log level>";
}

void parse_optval(char const* name, char const* in, loglevel_t& out)
{
  if(std::strcmp(in, "error") == 0)
  {
    out = loglevel_t::error;
  }
  else if(std::strcmp(in, "warning") == 0)
  {
    out = loglevel_t::warning;
  }
  else if(std::strcmp(in, "info") == 0)
  {
    out = loglevel_t::info;
  }
  else if(std::strcmp(in, "debug") == 0)
  {
    out = loglevel_t::debug;
  }
  else
  {
    exception_builder_t<std::runtime_error> builder;
    builder << "unexpected value '" << in << "' for option " << name <<
      "; valid values are 'error', 'warning', 'info' and 'debug'";
    builder.explode();
  }
}
    
logger_t::logger_t(char const* argv0)
: mutex_()
, backend_(new default_backend_t(argv0))
, n_failures_(0)
, first_failure_time_()
, first_failure_reason_()
{ }

void logger_t::set_backend(std::unique_ptr<logging_backend_t> backend)
{
  std::lock_guard<std::mutex> lock(mutex_);

  backend_ = std::move(backend);
}

void logger_t::report(loglevel_t level,
                      char const* begin_msg, char const* end_msg)
{
  std::lock_guard<std::mutex> lock(mutex_);

  static auto const max_failures = std::numeric_limits<unsigned int>::max();
  try
  {
    if(n_failures_ != 0 && backend_ != nullptr)
    {
      // report previous failures
      membuf_t buf;

      format_string(buf, "Logging failed at ");
      format_timepoint(buf, first_failure_time_);
      format_string(buf, ": ");
      format_string(buf, first_failure_reason_.c_str());
      format_string(buf, " - ");
      if(n_failures_ != max_failures)
      {
        format_unsigned(buf, n_failures_);
      }
      else
      {
        format_string(buf, "many");
      }
      format_string(buf, " message(s) lost");

      backend_->report(loglevel_t::error, buf.begin(), buf.end());
    }

    // if we're still here, clear failure mode
    n_failures_ = 0;

    if(backend_ != nullptr)
    {
      backend_->report(level, begin_msg, end_msg);
    }
  }
  catch(system_exception_t const& ex)
  {
    if(n_failures_ == 0)
    {
      // enter failure mode
      first_failure_time_ = std::chrono::system_clock::now();
      first_failure_reason_ = ex.what();
    }
    if(n_failures_ != max_failures)
    {
      // record failure
      ++n_failures_;
    }
  }
}

logger_t::~logger_t()
{ }

} // cuti
