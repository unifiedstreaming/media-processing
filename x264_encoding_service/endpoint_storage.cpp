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

#include "endpoint_storage.hpp"

#include "system_error.hpp"

#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netdb.h>
#endif

namespace xes
{

struct endpoint_storage_t::impl_t
{
  impl_t()
  { }

  impl_t(impl_t const&) = delete;
  impl_t& operator=(impl_t const&) = delete;

  virtual endpoint_t* get() = 0;

  virtual ~impl_t()
  { }
};

namespace // anonymous
{

template<int family>
struct family_specifics_t;

template<>
struct family_specifics_t<AF_INET>
{ using type = sockaddr_in; };

template<>
struct family_specifics_t<AF_INET6>
{ using type = sockaddr_in6; };

template<int family>
struct instance_t : endpoint_storage_t::impl_t
{
  instance_t()
  {
    std::memset(&data_, '\0', sizeof data_);
    data_.sockaddr_.sa_family = family;
  }

  endpoint_t* get() override
  { return &data_.sockaddr_; }

private :
  union
  {
    sockaddr sockaddr_;
    typename family_specifics_t<family>::type specifics_;
  } data_;
};

} // anonymous

endpoint_storage_t::endpoint_storage_t(int family)
: impl_()
{
  switch(family)
  {
  case AF_INET :
    impl_ = std::make_unique<instance_t<AF_INET>>();
    break;
  case AF_INET6 :
    impl_ = std::make_unique<instance_t<AF_INET6>>();
    break;
  default :
    throw system_exception_t("Address family " +
      std::to_string(family) + " not supported");
    break;
  }
}

endpoint_t* endpoint_storage_t::get()
{
  return impl_->get();
}

endpoint_t const* endpoint_storage_t::get() const
{
  return impl_->get();
}

endpoint_storage_t::~endpoint_storage_t()
{ }

} // xes
