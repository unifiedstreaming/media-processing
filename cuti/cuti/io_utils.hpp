/*
 * Copyright (C) 2022-2025 CodeShop B.V.
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

/*
 * Some portable, moderately useful I/O utiliies.
 */
namespace cuti
{

struct socket_layer_t;

CUTI_ABI bool is_wouldblock(socket_layer_t& sockets, int error);
CUTI_ABI bool is_fatal_io_error(socket_layer_t& sockets, int error);
CUTI_ABI void set_nonblocking(socket_layer_t& sockets, int fd, bool enable);

#if !defined(_WIN32)

/*
 * Race condition alert: setting the close-on-exec flag after opening
 * an fd will cause a descriptor leak if another thread calls fork()
 * in the meantime.  set_cloexec() is a last resort and should only be
 * used if the close-on-exec flag cannot be specified when the fd is
 * opened.
 */
CUTI_ABI void set_cloexec(socket_layer_t& sockets, int fd, bool enable);

#endif

} // cuti

#endif
