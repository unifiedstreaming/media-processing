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

struct addrinfo;
struct sockaddr;

namespace cuti
{

/*
 * Use a (scoped) socket_layer_t RAII object to get access to the
 * native socket API. Experience (under Windows) suggests that the
 * socket layer's lifetime should be nested in main() or one of its
 * subscopes.
 */
struct CUTI_ABI socket_layer_t
{
  socket_layer_t();

  socket_layer_t(socket_layer_t const&) = delete;
  socket_layer_t& operator=(socket_layer_t const&) = delete;

  ~socket_layer_t();

  /*
   * Address resolution
   */
  int getaddrinfo(char const* node, char const* service,
                  addrinfo const *hints, addrinfo** res);
  void freeaddrinfo(addrinfo* res);

#ifndef _WIN32
  char const* gai_strerror(int errcode);
#endif

 int getnameinfo(sockaddr const* addr, unsigned int addrlen,
                 char* host, unsigned int hostlen,
                 char* serv, unsigned int servlen,
                 int flags);

private :
  struct initializer_t;

  std::unique_ptr<initializer_t> initializer_;
};

} // cuti

#endif
