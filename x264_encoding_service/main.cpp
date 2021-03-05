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

#include "system_error.hpp"
#include "streambuf_logger.hpp"

#include <iostream>
#include <string>

int main()
{
  std::cout << "Started" << std::endl;
  {
    xes::streambuf_logger_t logger(std::cerr);
    try
    {
      int cause = 42;
      throw xes::system_exception_t("Can't open file", cause);
    }
    catch(std::exception const& ex)
    {
      logger.report(xes::loglevel_t::error, ex.what() + std::string(" (1)"));
      logger.report(xes::loglevel_t::warning, ex.what() + std::string(" (2)"));
      logger.report(xes::loglevel_t::info, ex.what() + std::string(" (3)"));
      logger.report(xes::loglevel_t::debug, ex.what() + std::string(" (4)"));
    }
  }
  std::cout << "Done" << std::endl;
  
  return 0;
}
