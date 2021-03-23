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

#ifndef SOCKET_NIFTY_HPP_
#define SOCKET_NIFTY_HPP_

namespace xes
{

/*
 * Nifty counter for initializing the native socket layer. Including
 * this file ensures the socket layer is initialized.
 */
struct socket_nifty_t
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

} // xes

#endif
