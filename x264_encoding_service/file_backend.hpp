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

#ifndef XES_FILE_BACKEND_HPP_
#define XES_FILE_BACKEND_HPP_

#include "logging_backend.hpp"

#include <memory>
#include <string>

namespace xes
{

/*
 * Logs to a named file.  Supports optional rotation-based purging
 * based on a file size limit and a count (rotation_depth) of old log
 * files that are kept around.  Old log files are named <filename>.1,
 * <filename>.2, etc.  Multiple file_backends addressing the same
 * filename will lead to spectacular behaviour when rotation is
 * enabled.
 */
struct file_backend_t : logging_backend_t
{
  struct log_handle_t;

  static const unsigned int no_size_limit_ = 0;
  static const unsigned int default_rotation_depth_ = 9;

  explicit file_backend_t(
    std::string const& filename,
    unsigned int size_limit = no_size_limit_,
    unsigned int rotation_depth = default_rotation_depth_);

  std::string const& effective_filename() const
  {
    return filename_;
  }

  void report(loglevel_t level,
              char const* begin_msg, char const* end_msg) override;

private :
  std::unique_ptr<log_handle_t> open_log_handle();
  
private :
  std::string const filename_;
  unsigned int const size_limit_;
  unsigned int const rotation_depth_;
  bool rotate_reported_;
};

} // namespace xes

#endif
