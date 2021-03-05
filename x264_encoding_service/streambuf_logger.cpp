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

#include "streambuf_logger.hpp"

#include <chrono>
#include <ctime>
#include <ostream>

#include <time.h> // for localtime_{s,r}

namespace xes
{

namespace // anonymous
{

const char *const day_names[] =
{
  "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

const char *const month_names[] =
{
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

#ifdef _WIN32

void get_localtime(std::tm& dst, const std::time_t& src)
{
  ::localtime_s(&dst, &src);
}

#else // POSIX

void get_localtime(std::tm& dst, const std::time_t& src)
{
  ::localtime_r(&src, &dst);
}

#endif

void format_unsigned(std::streambuf& target, unsigned int number, int width)
{
  if(number >= 10 || width > 1)
  {
    format_unsigned(target, number / 10, width - 1);
  }
  target.sputc(number % 10 + '0');
}

void format_string(std::streambuf& target, const char *str, int width)
{
  while(*str != '\0')
  {
    target.sputc(*str);
    ++str;
    --width;
  }

  while(width > 0)
  {
    target.sputc(' ');
    --width;
  }
}

void format_time(std::streambuf& target,
                 const std::chrono::system_clock::time_point& tp)
{
  auto duration = tp - std::chrono::system_clock::from_time_t(0);
  time_t secs = std::chrono::duration_cast
    <std::chrono::seconds>(duration).count();
  unsigned int msecs = std::chrono::duration_cast
    <std::chrono::milliseconds>(duration).count() % 1000;
  
  std::tm ltm;
  get_localtime(ltm, secs);

  format_string(target, day_names[ltm.tm_wday], 3);
  target.sputc(' ');
  format_unsigned(target, ltm.tm_year + 1900, 4);
  target.sputc('-');
  format_string(target, month_names[ltm.tm_mon], 3);
  target.sputc('-');
  format_unsigned(target, ltm.tm_mday, 2);
  target.sputc(' ');
  format_unsigned(target, ltm.tm_hour, 2);
  target.sputc(':');
  format_unsigned(target, ltm.tm_min, 2);
  target.sputc(':');
  format_unsigned(target, ltm.tm_sec, 2);
  target.sputc('.');
  format_unsigned(target, msecs, 3);
}

} // anonymous

streambuf_logger_t::streambuf_logger_t(std::streambuf* sb)
: mutex_()
, sb_(sb)
{ }

streambuf_logger_t::streambuf_logger_t(std::ostream& os)
: streambuf_logger_t(os.rdbuf())
{ }

void streambuf_logger_t::do_report(loglevel_t level, char const* message)
{
  if(sb_ == nullptr)
  {
    return;
  }

  std::lock_guard<std::mutex> guard(mutex_);

  auto now = std::chrono::system_clock::now();

  format_time(*sb_, now);
  sb_->sputc(' ');
  sb_->sputc('[');
  format_string(*sb_, loglevel_string(level), 7);
  sb_->sputc(']');
  sb_->sputc(' ');
  format_string(*sb_, message, 0);
  sb_->sputc('\n');

  sb_->pubsync();
}

} // namespace xes
