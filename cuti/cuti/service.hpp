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

#ifndef CUTI_SERVICE_HPP_
#define CUTI_SERVICE_HPP_

#include "linkage.h"

#include "logging_backend.hpp"
#include "logging_context.hpp"
#include "process_utils.hpp"

#include <memory>
#include <string>

namespace cuti
{

struct socket_layer_t;

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

  /*
   * Causes the current or next call to run() to return as soon as
   * possible.  The implementation of this function must be signal-
   * and thread-safe.
   */
  virtual void stop(int sig) = 0;

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

#ifndef _WIN32 // POSIX-only requirements

  /*
   * Tells if the service must be run as a daemon.
   */
  virtual bool run_as_daemon() const = 0;

  /*
   * Returns the user for the service or nullptr if no change is
   * required.
   */
  virtual user_t const* user() const = 0;

  /*
   * Returns the umask for the service or nullptr if no change is
   * required.
   */
  virtual umask_t const* umask() const = 0;

#endif // POSIX only

  /*
   * Returns the directory the service should change to, or nullptr
   * for no change.
   */
  virtual char const* directory() const = 0;

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
   * If this function returns nullptr, run_service() returns
   * immediately.
   */
  virtual std::unique_ptr<service_t>
  create_service(logging_context_t& logging_context) const = 0;

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
void run_service(socket_layer_t& sockets,
                 service_config_reader_t const& config_reader,
                 int argc, char const* const argv[]);

} // cuti

#endif
