/*
 * Copyright (C) 2021-2022 CodeShop B.V.
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

#include <cuti/service.hpp>
#include <x264_es_utils/config_reader.hpp>

#include <iostream>
#include <stdexcept>

namespace // anonymous
{

void throwing_main(int argc, char const* const argv[])
{
  x264_es_utils::config_reader_t config_reader;
  cuti::run_service(config_reader, argc, argv);
}

} // anonymous

int main(int argc, char* argv[])
{
  int result = 1;

  try
  {
    throwing_main(argc, argv);
    result = 0;
  }
  catch(std::exception const& ex)
  {
    std::cerr << argv[0] << ": " << ex.what() << std::endl;
  }

  return result;
}
