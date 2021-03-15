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

#ifndef SYSLOG_BACKEND_HPP_
#define SYSLOG_BACKEND_HPP_

#include "logging_backend.hpp"

#include <memory>

namespace xes
{

struct syslog_backend_t : logging_backend_t
{
  explicit syslog_backend_t(char const* source_name);

  void report(loglevel_t level,
              char const* begin_msg, char const* end_msg) override;

  ~syslog_backend_t();

private :
  struct impl_t;
  std::unique_ptr<impl_t> impl_;
};

} // namespace xes

#endif
