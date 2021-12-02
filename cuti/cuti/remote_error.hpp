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

#ifndef CUTI_REMOTE_ERROR_HPP_
#define CUTI_REMOTE_ERROR_HPP_

#include "identifier.hpp"
#include "linkage.h"
#include "tuple_mapping.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>

namespace cuti
{

struct CUTI_ABI remote_error_t : std::runtime_error
{
  remote_error_t(identifier_t type, std::string description)
  : std::runtime_error(make_message(type, description))
  , rep_(std::make_shared<rep_t>(std::move(type), std::move(description)))
  { }

  remote_error_t(remote_error_t const&) noexcept = default;
  remote_error_t& operator=(remote_error_t const&) noexcept = default;
  
  identifier_t const& type() const noexcept
  { return rep_->type_; }

  std::string const& description() const noexcept
  { return rep_->description_; }

private :
  static std::string make_message(identifier_t const& type,
                                  std::string const& description);

private :
  struct rep_t
  {
    rep_t(identifier_t type, std::string description);

    identifier_t type_;
    std::string description_;
  };

  std::shared_ptr<rep_t const> rep_; 
};

template<>
struct tuple_mapping_t<remote_error_t>
{
  using tuple_t = std::pair<identifier_t, std::string>;

  static auto to_tuple(remote_error_t const& error)
  {
    return tuple_t(error.type(), error.description());
  }

  static auto from_tuple(tuple_t t)
  {
    return std::make_from_tuple<remote_error_t>(std::move(t));
  }
};
    
} // cuti

#endif
