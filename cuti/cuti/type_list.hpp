/*
 * Copyright (C) 2021-2026 CodeShop B.V.
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

#ifndef CUTI_TYPE_LIST_HPP_
#define CUTI_TYPE_LIST_HPP_

namespace cuti
{

template<typename... Types>
struct type_list_t
{ };

template<template<typename...> typename ClassTemplate, typename TypeList>
struct bind_to_type_list_helper_t;

template<template<typename...> typename ClassTemplate, typename... Args>
struct bind_to_type_list_helper_t<ClassTemplate, type_list_t<Args...>>
{
  using type = ClassTemplate<Args...>;
};

template<template<typename...> typename ClassTemplate, typename TypeList>
using bind_to_type_list_t =
  typename bind_to_type_list_helper_t<ClassTemplate, TypeList>::type;

} // cuti

#endif
