/*
 * Copyright (C) 2021-2025 CodeShop B.V.
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

#include <cassert>
#include <csignal>
#include <memory>
#include <type_traits>
#include <utility>

namespace cuti
{

/*
 * signal_handler_t sets up a signal handler that calls a
 * user-provided handler when the OS reports some specific signal.
 * The usual platform-specific signal handler restrictions apply.
 *
 * Constructing or destroying a signal_handler_t while multiple
 * threads are running invokes undefined behavior.  Establish your
 * signal handlers before any threads are started, and only restore
 * them after these threads have been joined.
 *
 * Some signals may not be supported; on Windows, only SIGINT is
 * supported.
 *
 * Please note: for each signal, the lifetimes of the signal
 * handlers are assumed to nest; the signal handler constructed last
 * must be destroyed first.
 */
struct CUTI_ABI signal_handler_t
{
  struct impl_t;

  /*
   * Sets the signal handler for <sig>.  If <handler> is nullptr,
   * the signal is effectively ignored.
   *
   * For signal handlers, the stack_marker reference to passed to the
   * callback is a valid referece, but otherwise meaningless.
   */
  signal_handler_t(int sig, callback_t handler);

  signal_handler_t(signal_handler_t const&) = delete;
  signal_handler_t& operator=(signal_handler_t const&) = delete;

  /*
   * Restores any previously established signal handling for <sig>.
   */
  ~signal_handler_t();
  
private :
  std::unique_ptr<impl_t> impl_;
};

} // cuti

#endif
