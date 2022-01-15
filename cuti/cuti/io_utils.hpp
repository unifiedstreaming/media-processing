/*
 * Copyright (C) 2022 CodeShop B.V.
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

#ifndef CUTI_IO_UTILS_HPP_
#define CUTI_IO_UTILS_HPP_

#include "linkage.h"
#include "socket_nifty.hpp"

/*
 * Some portable, moderately useful I/O utiliies.
 */
namespace cuti
{

CUTI_ABI bool is_wouldblock(int error);
CUTI_ABI bool is_fatal_io_error(int error);
CUTI_ABI void set_nonblocking(int fd, bool enable);

#if !defined(_WIN32)

/*
 * Race condition alert: setting the close-on-exec flag after opening
 * an fd will cause a descriptor leak if another thread calls fork()
 * in the meantime.  set_cloexec() is a last resort and should only be
 * used if the close-on-exec flag cannot be specified when the fd is
 * opened.
 */
CUTI_ABI void set_cloexec(int fd, bool enable);

#endif

} // cuti

#endif
