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

#ifndef CUTI_ASYNC_SERIALIZERS_HPP_
#define CUTI_ASYNC_SERIALIZERS_HPP_

#include "async_source.hpp"
#include "parse_error.hpp"

#include <exception>

namespace cuti
{

namespace detail
{

struct check_eom_t
{
  template<typename Cont, typename... Args>
  void operator()(Cont&& cont, async_source_t source, Args&&... args) const
  {
    if(!source.readable())
    {
      source.call_when_readable(callback_t(*this, std::forward<Cont>(cont),
        source, std::forward<Args>(args)...));
      return;
    }

    if(source.peek() != '\n')
    {
      cont.fail(std::make_exception_ptr(parse_error_t("eof expected")));
      return;
    }

    source.skip();
    cont.submit(std::forward<Args>(args)...);
  }
};

} // namespace cuti::detail

inline auto constexpr check_eom = detail::check_eom_t{};

} // namespace cuti

#endif
