/*
 * Copyright (C) 2025 CodeShop B.V.
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

#ifndef CUTI_SOCKET_LAYER_HPP_
#define CUTI_SOCKET_LAYER_HPP_

#include "linkage.h"

#include <memory>

namespace cuti
{

/*
 * Use a (scoped) socket_layer_t RAII object to manage the lifetime of
 * the native socket layer.
 *
 * The native socket layer's lifetime should be managed from the main
 * executable. More specifically: it can *not* be managed by a
 * static-lifetime object living in a DLL. And yes, this is a quirk in
 * Windows.
 *
 * Functions and objects that require access to the native socket
 * layer should advertise this by taking a reference to a non-const
 * socket_layer_t.
 */
struct CUTI_ABI socket_layer_t
{
  socket_layer_t();

  socket_layer_t(socket_layer_t const&) = delete;
  socket_layer_t& operator=(socket_layer_t const&) = delete;

  ~socket_layer_t();

private :
  struct initializer_t;

  std::unique_ptr<initializer_t> initializer_;
};

} // cuti

#endif
