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

#ifndef FILE_LOGGER_HPP_
#define FILE_LOGGER_HPP_

#include "logger.hpp"

#include <chrono>
#include <memory>
#include <mutex>
#include <string>

namespace xes
{

/*
 * Logs to a named file.  Supports optional rotation-based purging
 * based on a file size limit and a count (rotation_depth) of old log
 * files that are kept around.  Old log files are named <filename>.1,
 * <filename>.2, etc.
 */
struct file_logger_t : logger_t
{
  struct log_handle_t;

  explicit file_logger_t(std::string filename,
                         unsigned int size_limit = 0,
                         unsigned int rotation_depth = 9);

  ~file_logger_t();

private :
  void do_report(loglevel_t level, char const* message) override;

private :
  std::unique_ptr<log_handle_t> open_log_handle();
  
private :
  std::string const filename_;
  unsigned int const size_limit_;
  unsigned int const rotation_depth_;

  std::mutex mutex_;
  bool rotating_;
  unsigned int n_failures_;
  std::chrono::system_clock::time_point first_failure_time_;
  std::string first_failure_reason_;
};

} // namespace xes

#endif
