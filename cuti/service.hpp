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

#ifndef CUTI_SERVICE_HPP_
#define CUTI_SERVICE_HPP_

#include "linkage.h"

#include "logging_backend.hpp"
#include "logging_context.hpp"
#include "process.hpp"
#include "tcp_connection.hpp"

#include <memory>
#include <string>

namespace cuti
{

/*
 * Abstract service application object interface.
 */
struct CUTI_ABI service_t
{
  service_t();

  service_t(service_t const&) = delete;
  service_t& operator=(service_t const&) = delete;

  /*
   * Runs the service, returning when done.
   */
  virtual void run() = 0;

  virtual ~service_t();
};

/*
 * Abstract service configuration object interface.
 */
struct CUTI_ABI service_config_t
{
  service_config_t();

  service_config_t(service_config_t const&) = delete;
  service_config_t& operator=(service_config_t const&) = delete;

#ifndef _WIN32
  /*
   * Tells if the service must be run as a daemon.  Only needed for
   * POSIX systems.
   */
  virtual bool run_as_daemon() const = 0;
#endif

  /*
   * Creates the logging backend to be used by the service.  If this
   * function returns nullptr, run_service() supplies a suitable
   * logging backend.
   */
  virtual std::unique_ptr<logging_backend_t>
  create_logging_backend() const = 0;

  /*
   * Returns a PID file holder for the service, or nullptr if no
   * PID file is required.
   */
  virtual std::unique_ptr<pidfile_t> create_pidfile() const = 0;
  
  /*
   * Creates the actual service application object.
   * The service should log via <logging_context>.
   * <control_connection> yields byte-sized shutdown signal
   * numbers; the service must respond to any input by returning from
   * its run() method.
   * If this function returns nullptr, run_service() returns
   * immediately.
   */
  virtual std::unique_ptr<service_t>
  create_service(logging_context_t& logging_context,
                 tcp_connection_t& control_connection) const = 0;

  virtual ~service_config_t();
};

/*
 * Abstract service configuration reader interface.
 */
struct CUTI_ABI service_config_reader_t
{
  service_config_reader_t();

  service_config_reader_t(service_config_reader_t const&) = delete;
  service_config_reader_t& operator=(service_config_reader_t const&) = delete;

  /*
   * Creates a service configuration by parsing the command line,
   * reporting errors by throwing an appropriate std::exception.
   * Must return a non-nullptr.
   */
  virtual std::unique_ptr<service_config_t>
  read_config(int argc, char const* const argv[]) const = 0;

  virtual ~service_config_reader_t();
};

/*
 * Reads the service configuration, creates the service object, and
 * and runs it.  Assumes the program will exit soon after returning
 * from this call.
 */
CUTI_ABI
void run_service(service_config_reader_t const& config_reader,
                 int argc, char const* const argv[]);

} // cuti

#endif
