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

#ifndef X264_CONFIG_HPP_
#define X264_CONFIG_HPP_

#include <cuti/endpoint.hpp>
#include <cuti/flag.hpp>
#include <cuti/fs_utils.hpp>
#include <cuti/loglevel.hpp>
#include <cuti/selector_factory.hpp>
#include <cuti/service.hpp>

#include <optional>
#include <ostream>
#include <string>
#include <vector>

namespace cuti
{

struct args_reader_t;

} // cuti

struct x264_config_t : cuti::service_config_t
{
  x264_config_t(int argc, char const* const argv[]);

#ifndef _WIN32
  bool run_as_daemon() const override;
  cuti::group_id_t const* group_id() const override;
  cuti::user_id_t const* user_id() const override;
  cuti::umask_t const* umask() const override;
#endif

  char const* directory() const override;

  std::unique_ptr<cuti::logging_backend_t>
  create_logging_backend() const override;

  std::unique_ptr<cuti::pidfile_t>
  create_pidfile() const override;

  std::unique_ptr<cuti::service_t>
  create_service(cuti::logging_context_t& context,
                 cuti::tcp_connection_t& control_connection) const override;

  ~x264_config_t() override;

private :
  void read_options(cuti::args_reader_t& reader);
  void read_options(cuti::args_reader_t& reader, int config_file_depth);
  void print_usage(std::ostream& os);

private :
  static constexpr cuti::loglevel_t default_loglevel = 
    cuti::loglevel_t::warning;

  std::string argv0_;

#ifndef _WIN32
  cuti::flag_t daemon_;
#endif
  std::string directory_;
  cuti::flag_t dry_run_;
  std::vector<cuti::endpoint_t> endpoints_;
#ifndef _WIN32
  std::optional<cuti::group_id_t> group_;
#endif
  cuti::absolute_path_t logfile_;
  cuti::loglevel_t loglevel_;
  cuti::absolute_path_t pidfile_;
  unsigned int rotation_depth_;
  cuti::selector_factory_t selector_;
  unsigned int size_limit_;
  cuti::flag_t syslog_;  
  std::string syslog_name_;
#ifndef _WIN32
  std::optional<cuti::umask_t> umask_;
  std::optional<cuti::user_id_t> user_;
#endif
};

#endif