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

#ifndef STREAMBUF_LOGGER_HPP_
#define STREAMBUF_LOGGER_HPP_

#include "logger.hpp"

#include <mutex>
#include <iosfwd>
#include <streambuf>

namespace xes
{

/*
 * This logger provides concurrent logging to an existing ostream or
 * streambuf.
 * Please note that while this class uses a mutex to project the
 * target streambuf, it cannot project the target streambuf from
 * concurrent writes that bypass this mutex.  Depending on how the
 * streambuf is used, such writes may lead to a data race (==UB) or
 * garbled output otherwise.  See the C++14 standard, clauses 27.2.3
 * and and 27.4.1.
 */
struct streambuf_logger_t : logger_t
{
  explicit streambuf_logger_t(std::streambuf* sb);
  explicit streambuf_logger_t(std::ostream& os);

private :
  virtual void do_report(loglevel_t level, char const* message) override;

private :
  std::mutex mutex_;
  std::streambuf* const sb_;
};

} // namespace xes

#endif
