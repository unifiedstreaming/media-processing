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

#ifndef XES_LOGGING_CONTEXT_HPP_
#define XES_LOGGING_CONTEXT_HPP_

#include "logger.hpp"
#include "membuf.hpp"

#include <memory>
#include <ostream>

namespace xes
{

struct log_message_t : std::ostream
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

struct logging_context_t
{
  logging_context_t(logger_t& logger, loglevel_t level)
  : logger_(logger)
  , level_(level)
  { }

  std::unique_ptr<log_message_t> message_at(loglevel_t level) const
  {
    std::unique_ptr<log_message_t> result = nullptr;
    if(level_ >= level)
    {
      result.reset(new log_message_t(logger_, level));
    }
    return result;
  }
      
private :
  logger_t& logger_;
  loglevel_t level_;
};

} // xes

#endif