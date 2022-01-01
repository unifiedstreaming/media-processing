/*
 * Copyright (C) 2021-2022 CodeShop B.V.
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

#include "logging_context.hpp"

namespace cuti
{

log_message_t::log_message_t(logger_t& logger, loglevel_t level)
: std::ostream(nullptr)
, logger_(logger)
, level_(level)
, buf_()
{
  this->rdbuf(&buf_);
}

log_message_t::~log_message_t()
{
  logger_.report(level_, buf_.begin(),  buf_.end());
}

} // cuti

