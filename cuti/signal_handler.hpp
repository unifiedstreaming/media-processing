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

#ifndef CUTI_SIGNAL_HANDLER_HPP_
#define CUTI_SIGNAL_HANDLER_HPP_

#include "callback.hpp"
#include "linkage.h"

#include <csignal>
#include <memory>

namespace cuti
{

/*
 * signal_handler_t sets up a signal handler that calls a
 * user-provided callback when the OS reports some specific signal.
 * If the callback is specified as nullptr, the signal is ignored.
 *
 * At any time, for each signal, at most one instance of
 * signal_handler_t may be alive.  The callbacks for different
 * signals do not race against each other; other than that, the
 * usual platform-specific signal handler restrictions apply.
 *
 * Some signals may not be supported; on Windows, only SIGINT is
 * supported.
 */
struct CUTI_ABI signal_handler_t
{
  struct impl_t;

  signal_handler_t(int sig, callback_t callback);

  signal_handler_t(signal_handler_t const&)
    = delete;
  signal_handler_t& operator=(signal_handler_t const&)
    = delete;

  /*
   * Restores any previously established signal handler.
   */
  ~signal_handler_t();
  
private :
  std::unique_ptr<impl_t> impl_;
};

} // cuti

#endif
