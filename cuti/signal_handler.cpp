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

#include "signal_handler.hpp"

#include "scoped_guard.hpp"
#include "system_error.hpp"

#include <signal.h>
#include <utility>

#ifdef _WIN32
#include <windows.h>
#else
#include <cstring>
#include <errno.h>
#endif

// enable assertions here, even in release mode
#undef NEBUG
#include <cassert>

namespace cuti
{

namespace // anonymous
{

constexpr int n_sigs = 32;
signal_handler_t::impl_t const* impls[n_sigs];

} // anonymous

#ifdef _WIN32

struct signal_handler_t::impl_t
{
  impl_t(int sig, callback_t callback)
  : callback_(std::move(callback))
  {
    if(sig != SIGINT)
    {
      system_exception_builder_t builder;
      builder << "signal_handler_t(): unsupported signal " << sig;
      builder.explode();
    }

    assert(impls[SIGINT] == nullptr);
    impls[SIGINT] = this;
    auto impl_guard = make_scoped_guard([&] { impls[SIGINT] = nullptr; });
      
    BOOL r = SetConsoleCtrlHandler(
      callback_ != nullptr ? handler : NULL, TRUE);
    if(r == 0)
    {
      int cause = last_system_error();
      throw system_exception_t("SetConsoleCtrlHandler() failure", cause);
    }

    impl_guard.dismiss();
  }

  impl_t(impl_t const&) = delete;
  impl_t& operator=(impl_t const&) = delete;
  
  ~impl_t()
  {
    BOOL r = SetConsoleCtrlHandler(NULL, FALSE);
    assert(r != 0);

    assert(impls[SIGINT] == this);
    impls[SIGINT] = nullptr;
  };
    
private :
  static BOOL WINAPI handler(DWORD dwCtrlType) noexcept
  {
    BOOL result = FALSE;

    if(dwCtrlType == CTRL_C_EVENT)
    {
      assert(impls[SIGINT] != nullptr);
      assert(impls[SIGINT]->callback_ != nullptr);
      impls[SIGINT]->callback_();
      result = TRUE;
    }

    return result;
  }

private :
  callback_t const callback_;
}; 

#else // POSIX

struct signal_handler_t::impl_t
{
  impl_t(int sig, callback_t callback)
  : sig_(sig)
  , callback_(std::move(callback))
  {
    if(sig_ < 0 || sig_ >= n_sigs)
    {
      system_exception_builder_t builder;
      builder << "signal_handler_t(): unsupported signal " << sig_;
      builder.explode();
    }

    assert(impls[sig_] == nullptr);
    impls[sig_] = this;
    auto impl_guard = make_scoped_guard([&] { impls[sig_] = nullptr; });
      
    struct sigaction new_action;
    std::memset(&new_action, '\0', sizeof new_action);
    new_action.sa_handler = callback_ != nullptr ? handler : SIG_IGN;
    sigfillset(&new_action.sa_mask);
    new_action.sa_flags = SA_RESTART;

    int r = sigaction(sig_, &new_action, &prev_action_);
    if( r == -1)
    {
      int cause = last_system_error();
      throw system_exception_t("sigaction() failure", cause);
    }

    impl_guard.dismiss();
  }
    
  impl_t(impl_t const&) = delete;
  impl_t& operator=(impl_t const&) = delete;
  
  ~impl_t()
  {
    int r = sigaction(sig_, &prev_action_, nullptr);
    assert(r != -1);

    assert(impls[sig_] == this);
    impls[sig_] = nullptr;
  }

private :
  static void handler(int sig) noexcept
  {
    assert(sig >= 0);
    assert(sig < n_sigs);
    assert(impls[sig] != nullptr);
    assert(impls[sig]->callback_ != nullptr);

    auto saved_errno = errno;
    impls[sig]->callback_();
    errno = saved_errno;
  }

private :
  int sig_;
  callback_t const callback_;
  struct sigaction prev_action_;
};

#endif // POSIX

signal_handler_t::signal_handler_t(int sig, callback_t callback)
: impl_(std::make_unique<impl_t>(sig, std::move(callback)))
{ }

signal_handler_t::~signal_handler_t()
{ }

} // cuti
