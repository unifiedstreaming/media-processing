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

#ifndef SYSTEM_LOGGER_HPP_
#define SYSTEM_LOGGER_HPP_

#include "logger.hpp"

#include <memory>

namespace xes
{

struct system_logger_t : logger_t
{
  explicit system_logger_t(char const* source_name);
  ~system_logger_t();

private :
  void do_report(loglevel_t level, char const* message) override;

private :
  struct impl_t;
  std::unique_ptr<impl_t> impl_;
};

} // namespace xes

#endif
