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

#ifndef CUTI_LOGGING_CONTEXT_HPP_
#define CUTI_LOGGING_CONTEXT_HPP_

#include "linkage.h"
#include "logger.hpp"
#include "membuf.hpp"

#include <atomic>
#include <memory>
#include <ostream>

namespace cuti
{

struct CUTI_ABI log_message_t : std::ostream
{
  log_message_t(logger_t& logger, loglevel_t level);

  log_message_t(log_message_t const&) = delete;
  log_message_t& operator=(log_message_t const&) = delete;

  ~log_message_t() override;

private :
  logger_t& logger_;
  loglevel_t level_;
  membuf_t buf_;
};

struct CUTI_ABI logging_context_t
{
  logging_context_t(logger_t& logger, loglevel_t level) noexcept
  : logger_(logger)
  , level_(level)
  { }

  loglevel_t level() const noexcept
  { return level_.load(std::memory_order_acquire); }

  void level(loglevel_t level) noexcept
  { level_.store(level, std::memory_order_release); }

  std::unique_ptr<log_message_t> message_at(loglevel_t at_level) const
  {
    std::unique_ptr<log_message_t> result = nullptr;

    auto curr_level = this->level();
    if(curr_level >= at_level)
    {
      result.reset(new log_message_t(logger_, at_level));
    }

    return result;
  }

private :
  logger_t& logger_;
  std::atomic<loglevel_t> level_;
};

} // cuti

#endif
