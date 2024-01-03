/*
 * Copyright (C) 2021-2024 CodeShop B.V.
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

#ifndef CUTI_SOCKET_NIFTY_HPP_
#define CUTI_SOCKET_NIFTY_HPP_

#include "linkage.h"

namespace cuti
{

struct socket_initializer_t;

/*
 * Nifty counter for initializing the native socket layer. Including
 * this file ensures the socket layer is initialized.
 */
struct CUTI_ABI socket_nifty_t
{
  socket_nifty_t();

  socket_nifty_t(socket_nifty_t const&) = delete;
  socket_nifty_t& operator=(socket_nifty_t const&) = delete;

  ~socket_nifty_t();
};

namespace // anonymous
{

socket_nifty_t this_files_socket_nifty;

} // anonymous

} // cuti

#endif
