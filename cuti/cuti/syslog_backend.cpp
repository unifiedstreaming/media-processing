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

#include "syslog_backend.hpp"

#include "format.hpp"
#include "membuf.hpp"
#include "system_error.hpp"

#ifdef _WIN32

#include <windows.h>

namespace cuti
{

struct syslog_backend_t::impl_t
{
  explicit impl_t(std::string const& source_name)
  : handle_(RegisterEventSource(nullptr, source_name.c_str()))
  {
    if(handle_ == nullptr)
    {
      auto cause = last_system_error();
      throw system_exception_t("Can't create system logger", cause);
    }
  }

  impl_t(impl_t const&) = delete;
  impl_t& operator=(impl_t const&) = delete;

  void report(loglevel_t level, char const* message)
  {
    char const* strings[1];
    strings[0] = message;

    BOOL result = ReportEvent(handle_, loglevel_type(level), 0, 0,
                              nullptr, 1, 0, strings, nullptr);
    if(!result)
    {
      int cause = last_system_error();
      throw system_exception_t("ReportEvent() failure", cause);
    }
  }

  ~impl_t()
  {
    DeregisterEventSource(handle_);
  }

private :
  static WORD loglevel_type(loglevel_t level)
  {
    switch(level)
    {
    case loglevel_t::error : return EVENTLOG_ERROR_TYPE;
    case loglevel_t::warning : return EVENTLOG_WARNING_TYPE;
    case loglevel_t::info : return EVENTLOG_INFORMATION_TYPE;
    case loglevel_t::debug : return EVENTLOG_SUCCESS;
    }
    return EVENTLOG_ERROR_TYPE;
  }

private :
  HANDLE handle_;
};

} // namespace cuti

#else // POSIX

#include <syslog.h>

namespace cuti
{

struct syslog_backend_t::impl_t
{
  explicit impl_t(std::string const& source_name)
  : source_name_(source_name)
  {
    openlog(source_name_.c_str(), 0, LOG_USER);
  }

  impl_t(impl_t const&) = delete;
  impl_t& operator=(impl_t const&) = delete;

  void report(loglevel_t level, char const* message)
  {
    syslog(priority(level), "%s", message);
  }

  ~impl_t()
  {
    closelog();
  }

private :
  static int priority(loglevel_t level)
  {
    switch(level)
    {
    case loglevel_t::error : return LOG_ERR;
    case loglevel_t::warning : return LOG_WARNING;
    case loglevel_t::info : return LOG_INFO;
    case loglevel_t::debug : return LOG_DEBUG;
    }
    return LOG_ERR;
  }

private :
  std::string source_name_;
};

} // namespace cuti

#endif

namespace cuti
{

syslog_backend_t::syslog_backend_t(std::string const& source_name)
: impl_(new impl_t(source_name))
{ }

void syslog_backend_t::report(loglevel_t level,
                              char const* begin_msg, char const* end_msg)
{
  membuf_t buf;
  format_loglevel(buf, level);
  buf.sputc(' ');
  buf.sputn(begin_msg, end_msg - begin_msg);
  buf.sputc('\0');
  impl_->report(level, buf.begin());
}

syslog_backend_t::~syslog_backend_t()
{ }

std::string default_syslog_name(char const* argv0)
{
  char const* last_segment = argv0;
  char const* last_dot = nullptr;

  char const *p;
  for(p = argv0; *p != '\0'; ++p)
  {
    switch(*p)
    {
    case '/' :
#ifdef _WIN32
    case ':' :
    case '\\' :
#endif
      last_segment = p + 1;
      last_dot = nullptr;
      break;
    case '.' :
      last_dot = p;
      break;
    default :
      break;
    }
  }

  if(last_dot == nullptr)
  {
    last_dot = p;
  }

  return std::string(last_segment, last_dot);
}

} // namespace cuti
