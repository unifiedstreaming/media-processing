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

#include "boolean_readers.hpp"

#include "parse_error.hpp"

#include <cassert>
#include <utility>

namespace cuti
{

namespace detail
{

template<typename T>
boolean_reader_t<T>::boolean_reader_t(result_t<T>& result, bound_inbuf_t& buf)
: result_(result)
, buf_(buf)
, finder_(*this, &boolean_reader_t::on_failure, buf_)
{ }

template<typename T>
void boolean_reader_t<T>::start()
{
  finder_.start(&boolean_reader_t::on_begin_token);
}

template<typename T>
void boolean_reader_t<T>::on_begin_token(int c)
{
  assert(buf_.readable());
  assert(buf_.peek() == c);

  T value{};
  switch(c)
  {
  case '!' :
    value = false;
    break;
  case '*' :
    value = true;
    break;
  default :
    result_.fail(parse_error_t("boolean value (\'!\' or \'*\') expected"));
    return;
  }
  buf_.skip();

  result_.submit(value);
}
  
template<typename T>
void boolean_reader_t<T>::on_failure(std::exception_ptr ex)
{
  result_.fail(std::move(ex));
}

template struct boolean_reader_t<bool>;
template struct boolean_reader_t<flag_t>;

} // detail

} // cuti
