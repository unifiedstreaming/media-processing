/*
 * Copyright (C) 2021 CodeShop B.V.
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

#ifndef CUTI_FILE_BACKEND_HPP_
#define CUTI_FILE_BACKEND_HPP_

#include "linkage.h"

#include "fs_utils.hpp"
#include "logging_backend.hpp"

#include <memory>
#include <string>

namespace cuti
{

struct text_output_file_t;

/*
 * Logs to a named file.  Supports optional rotation-based purging
 * based on a file size limit and a count (rotation_depth) of old log
 * files that are kept around.  Old log files are named <filename>.1,
 * <filename>.2, etc.  Multiple file_backends addressing the same
 * filename will lead to spectacular behaviour when rotation is
 * enabled.
 */
struct CUTI_ABI file_backend_t : logging_backend_t
{
  static constexpr unsigned int no_size_limit = 0;
  static constexpr unsigned int default_rotation_depth = 9;

  explicit file_backend_t(
    absolute_path_t path,
    unsigned int size_limit = no_size_limit,
    unsigned int rotation_depth = default_rotation_depth);

  void report(loglevel_t level,
              char const* begin_msg, char const* end_msg) override;

private :
  std::unique_ptr<text_output_file_t> open_log_handle();

private :
  absolute_path_t path_;
  unsigned int const size_limit_;
  unsigned int const rotation_depth_;
  bool rotate_reported_;
};

} // namespace cuti

#endif
