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

#include "system_error.hpp"

#include <signal.h>
#include <utility>

#ifdef _WIN32
#include <mutex>
#include <windows.h>
#else
#include <cstring>
#include <errno.h>
#endif

// enable assertions here, even in release mode
#undef NDEBUG
#include <cassert>

namespace cuti
{

#ifdef _WIN32

namespace // anonymous
{

/*
 * Under Windows, the console control handler is called from a hidden,
 * OS-level thread; use a std::mutex to protect our implementation
 * pointer.
 */
std::mutex curr_impl_mutex;
signal_handler_t::impl_t const* curr_impl = nullptr;

} // anonymous

struct signal_handler_t::impl_t
{
  impl_t(int sig, callback_t callback)
  : callback_(std::move(callback))
  , prev_impl_(nullptr)
  {
    if(sig != SIGINT)
    {
      system_exception_builder_t builder;
      builder << "signal_handler_t(): unsupported signal " << sig;
      builder.explode();
    }

    // First setup curr_impl...
    {
      std::scoped_lock<std::mutex> lock(curr_impl_mutex);
      prev_impl_ = curr_impl;
      curr_impl = this;
    }

    // ...then add the handler
    BOOL r = SetConsoleCtrlHandler(handler, TRUE);
    assert(r != 0);
  }

  impl_t(impl_t const&) = delete;
  impl_t& operator=(impl_t const&) = delete;
  
  ~impl_t()
  {
    // First remove the handler..
    BOOL r = SetConsoleCtrlHandler(handler, FALSE);
    assert(r != 0);
    
    // ...then restore curr_impl
    {
      std::scoped_lock<std::mutex> lock(curr_impl_mutex);
      assert(curr_impl == this);
      curr_impl = prev_impl_;
    }
  }

private :
  static BOOL WINAPI handler(DWORD dwCtrlType) noexcept
  {
    BOOL result = FALSE;

    if(dwCtrlType == CTRL_C_EVENT)
    {
      std::scoped_lock<std::mutex> lock(curr_impl_mutex);
      assert(curr_impl != nullptr);

      if(curr_impl->callback_ != nullptr)
      {
        curr_impl->callback_();
      }

      result = TRUE;
    }

    return result;
  }

private :
  callback_t callback_;
  impl_t const *prev_impl_;
}; 

#else // POSIX

namespace // anonymous
{

struct signal_blocker_t
{
  explicit signal_blocker_t(int sig)
  {
    sigset_t new_set;
    sigemptyset(&new_set);
    sigaddset(&new_set, sig);
    int r = sigprocmask(SIG_BLOCK, &new_set, &prev_set_);
    assert(r == 0);
  }

  signal_blocker_t(signal_blocker_t const&) = delete;
  signal_blocker_t& operator=(signal_blocker_t const&) = delete;

  ~signal_blocker_t()
  {
    int r = sigprocmask(SIG_SETMASK, &prev_set_, nullptr);
    assert(r == 0);
  }

private :
  sigset_t prev_set_;
};
    
constexpr int n_sigs = 32;
signal_handler_t::impl_t const* curr_impls[n_sigs];

} // anonymous

struct signal_handler_t::impl_t
{
  impl_t(int sig, callback_t callback)
  : sig_(sig)
  , callback_(std::move(callback))
  , prev_impl_(nullptr)
  {
    if(sig_ < 0 || sig_ >= n_sigs)
    {
      system_exception_builder_t builder;
      builder << "signal_handler_t(): unsupported signal " << sig_;
      builder.explode();
    }

    // First setup curr_impls[sig_]...
    {
      signal_blocker_t blocker(sig_);
      prev_impl_ = curr_impls[sig_];
      curr_impls[sig_] = this;
    }

    // ...then redirect the handler
    struct sigaction new_action;
    std::memset(&new_action, '\0', sizeof new_action);

    new_action.sa_handler = handler;
    sigemptyset(&new_action.sa_mask);
    sigaddset(&new_action.sa_mask, sig_);
    new_action.sa_flags = SA_RESTART;

    int r = sigaction(sig_, &new_action, &prev_action_);
    assert(r == 0);
  }
    
  impl_t(impl_t const&) = delete;
  impl_t& operator=(impl_t const&) = delete;
  
  ~impl_t()
  {
    // First restore the handler...
    int r = sigaction(sig_, &prev_action_, nullptr);
    assert(r == 0);

    // ...then restore curr_impls[sig_]
    {
      signal_blocker_t blocker(sig_);
      assert(curr_impls[sig_] == this);
      curr_impls[sig_] = prev_impl_;
    }
  }

private :
  static void handler(int sig) noexcept
  {
    assert(sig >= 0);
    assert(sig < n_sigs);
    assert(curr_impls[sig] != nullptr);

    if(curr_impls[sig]->callback_ != nullptr)
    {
      auto saved_errno = errno;
      curr_impls[sig]->callback_();
      errno = saved_errno;
    }
  }

private :
  int sig_;
  callback_t callback_;
  impl_t const* prev_impl_;
  struct sigaction prev_action_;
};

#endif // POSIX

signal_handler_t::signal_handler_t(int sig, callback_t callback)
: impl_(std::make_unique<impl_t>(sig, std::move(callback)))
{ }

signal_handler_t::~signal_handler_t()
{ }

} // cuti
