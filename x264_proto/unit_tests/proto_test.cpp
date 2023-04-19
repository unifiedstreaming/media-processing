/*
 * Copyright (C) 2023 CodeShop B.V.
 *
 * This file is part of the x264_proto library.
 *
 * The x264_proto library is free software: you can redistribute it
 * and/or modify it under the terms of version 2 of the GNU General
 * Public License as published by the Free Software Foundation.
 *
 * The x264_proto library is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See version 2 of the GNU General Public License for more details.
 *
 * You should have received a copy of version 2 of the GNU General
 * Public License along with the x264_proto library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <cuti/cmdline_reader.hpp>
#include <cuti/dispatcher.hpp>
#include <cuti/endpoint.hpp>
#include <cuti/flag.hpp>
#include <cuti/logger.hpp>
#include <cuti/logging_context.hpp>
#include <cuti/option_walker.hpp>
#include <cuti/resolver.hpp>
#include <cuti/rpc_client.hpp>
#include <cuti/scoped_guard.hpp>
#include <cuti/scoped_thread.hpp>
#include <cuti/streambuf_backend.hpp>
#include <x264_proto/client.hpp>

#include <csignal>
#include <exception>
#include <iostream>

#undef NDEBUG
#include <cassert>

namespace // anonymous
{

int run_tests(int argc, char const* const* argv)
{
  return 0;
}

} // anonymous

int main(int argc, char* argv[])
{
  try
  {
    return run_tests(argc, argv);
  }
  catch(std::exception const& ex)
  {
    std::cerr << argv[0] << ": exception: " << ex.what() << std::endl;
  }

  return 1;
}
