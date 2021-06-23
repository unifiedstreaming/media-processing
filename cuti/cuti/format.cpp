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

#include "format.hpp"

#include <ctime>
#include <time.h> // for localtime_{s,r}

namespace cuti
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

} // anonymous

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

void format_loglevel(std::streambuf& target, loglevel_t level)
{
  target.sputc('[');
  format_string(target, loglevel_string(level), 7);
  target.sputc(']');
}

void format_time_point(std::streambuf& target, time_point_t tp)
{
  auto duration = tp - cuti_clock_t::from_time_t(0);
  time_t secs = duration_cast<seconds_t>(duration).count();
  unsigned int msecs = duration_cast<milliseconds_t>(duration).count() % 1000;

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

} // namespace cuti
