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

#ifndef XES_STREAMBUF_BACKEND_HPP_
#define XES_STREAMBUF_BACKEND_HPP_

#include "logging_backend.hpp"

#include <iosfwd>
#include <streambuf>

namespace xes
{

/*
 * This backend provides logging to an existing ostream or streambuf.
 * Please note that, unless its owning ostream is synchronized,
 * concurrent writes to the target streambuf that bypass the logger
 * framework lead to a data race (==UB).
 * std::cout, std::cerr and std::clog are synchronized when
 * std::ios_base::sync_with_stdio(false) is not called; no other
 * ostreams are synchronized.  See the C++14 standard, clauses 27.2.3
 * and and 27.4.1.
 */
struct streambuf_backend_t : logging_backend_t
{
  explicit streambuf_backend_t(std::streambuf* sb);
  explicit streambuf_backend_t(std::ostream& os);

  void report(loglevel_t level,
              char const* begin_msg, char const* end_msg) override;

private :
  std::streambuf* const sb_;
};

} // namespace xes

#endif
