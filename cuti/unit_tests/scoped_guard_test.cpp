/*
 * Copyright (C) 2021-2022 CodeShop B.V.
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

#include <cuti/scoped_guard.hpp>

#include <iostream>
#include <exception>

// Enable assert()
#undef NDEBUG
#include <cassert>

namespace // anonymous
{

using namespace cuti;

void not_dismissed()
{
  unsigned int n_calls = 0;
  {
    auto guard = make_scoped_guard([&] { ++n_calls; });
  }
  assert(n_calls == 1);
}

void dismissed()
{
  unsigned int n_calls = 0;
  {
    auto guard = make_scoped_guard([&] { ++n_calls; });
    guard.dismiss();
  }
  assert(n_calls == 0);
}

void lvalue_lambda()
{
  unsigned int n_calls = 0;
  {
    auto cleanup = [&] { ++n_calls; };
    auto guard1 = make_scoped_guard(cleanup);
    auto guard2 = make_scoped_guard(cleanup);
  }
  assert(n_calls == 2);
}

void run_tests(int, char const* const*)
{
  not_dismissed();
  dismissed();
  lvalue_lambda();
}

} // anonymous

int main(int argc, char* argv[])
{
  try
  {
    run_tests(argc, argv);
  }
  catch(std::exception const& ex)
  {
    std::cerr << argv[0] << ": exception: " << ex.what() << std::endl;
    throw;
  }

  return 0;
}
