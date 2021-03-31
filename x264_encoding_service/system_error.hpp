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

#ifndef XES_SYSTEM_ERROR_HPP_
#define XES_SYSTEM_ERROR_HPP_

#include <ostream>
#include <stdexcept>
#include <string>

#include "membuf.hpp"

namespace xes
{

int last_system_error();
bool is_wouldblock(int error);
std::string system_error_string(int error);

struct system_exception_t : std::runtime_error
{
  explicit system_exception_t(std::string complaint);
  system_exception_t(std::string complaint, int cause);
};

struct system_exception_builder_t : std::ostream
{
  system_exception_builder_t()
  : std::ostream(nullptr)
  , buf_()
  { this->rdbuf(&buf_); }

  system_exception_builder_t(system_exception_builder_t const&) = delete;
  system_exception_builder_t& operator=(system_exception_builder_t const&)
    = delete;

  void explode() const
  { throw system_exception_t(std::string(buf_.begin(), buf_.end())); }

  void explode(int cause) const
  { throw system_exception_t(std::string(buf_.begin(), buf_.end()), cause); }

private :
  membuf_t buf_;
};

} // namespace xes

#endif
