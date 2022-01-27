/*
 * Copyright (C) 2022 CodeShop B.V.
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

#include <cuti/cmdline_reader.hpp>
#include <cuti/endpoint.hpp>
#include <cuti/option_walker.hpp>
#include <cuti/system_error.hpp>
#include <cuti/tcp_connection.hpp>

#include <iostream>

namespace // anonymous
{

using namespace cuti;

struct options_t
{
  options_t()
  : target_()
  { }

  endpoint_t target_;
};

void print_usage(std::ostream& os, char const* argv0)
{
  os << "usage: " << argv0 << " [<option> ...]\n";
  os << "options are:\n";
  os << "  --target <port>@<ip>  specifies target endpoint\n";
  os << std::flush;
}

void read_options(options_t& options, option_walker_t& walker)
{
  while(!walker.done())
  {
    if(!walker.match(
         "--target", options.target_))
    {
      break;
    }
  }
}

int real_main(int argc, char const* const* argv)
{
  options_t options;
  cmdline_reader_t reader(argc, argv);
  option_walker_t walker(reader);

  read_options(options, walker);
  if(!walker.done() || !reader.at_end())
  {
    print_usage(std::cerr, argv[0]);
    return 1;
  }

  if(!options.target_.empty())
  {
    tcp_connection_t conn(options.target_);
    std::cout << "connected: " << conn << std::endl;

    std::string const s = "y\n";
    for(;;)
    {
      char const* first = s.data();
      char const* last = first + s.length();
      while(first != last)
      {
        char const* next;
        auto error = conn.write(first, last, next);
        if(error != 0)
        {
          throw system_exception_t("write error", error);
        }
        assert(next != nullptr);
        first = next;
      }
    }
  }

  return 0;
}

} // anonymous

int main(int argc, char* argv[])
{
  try
  {
    return real_main(argc, argv);
  }
  catch(std::exception const& ex)
  {
    std::cerr << argv[0] << ": exception: " << ex.what() << std::endl;
  }

  return 1;
}
