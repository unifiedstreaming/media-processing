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

#ifndef CUTI_EVENT_MANAGER_HPP_
#define CUTI_EVENT_MANAGER_HPP_

#include "callback.hpp"

#include <cassert>
#include <functional>
#include <utility>

namespace cuti
{

/*
 * An event manager manages a one-off subscription to an event
 * notification as defined by its event adapter type.  It alternates
 * between its intitial state (no user callback requested), and its
 * active state (user callback requested).  We revert to the intial
 * state when (1) the user cancels the callback or (2) the adapter
 * reports the event and the user callback is invoked.
 */
template<typename EventAdapter>
struct event_manager_t
{
  using event_adapter_t = EventAdapter;
  using scheduler_t = typename event_adapter_t::scheduler_t;

  event_manager_t()
  : adapter_()
  , adapter_callback_([this] { this->on_adapter_callback(); })
  , current_scheduler_(nullptr)
  , current_ticket_(-1)
  , current_user_callback_(nullptr)
  { }

  explicit event_manager_t(event_adapter_t adapter)
  : adapter_(std::move(adapter))
  , adapter_callback_([this] { this->on_adapter_callback(); })
  , current_scheduler_(nullptr)
  , current_ticket_(-1)
  , current_user_callback_(nullptr)
  { }

  event_manager_t(event_manager_t const&) = delete;
  event_manager_t& operator=(event_manager_t const&) = delete;

  /*
   * Requests a single user callback for when <scheduler> reports the
   * event defined by the event adapter, canceling any previously set
   * callback.  Zero or more user-specified <args> (which are used to
   * qualify the event) are simply forwarded to the adapter.
   */
  template<typename F, typename... Args>
  void set(F&& callback, scheduler_t& scheduler, Args&&... args)
  {
    /*
     * Cancel any previous adapter ticket; this leaves us in the
     * initial state.
     */
    reset();

    /*
     * Package the users's callback into a callback_t.
     */
    callback_t packaged_callback(std::forward<F>(callback));
    assert(packaged_callback != nullptr);

    /*
     * Get a fresh callback ticket from the adapter; note that we
     * simply (re)use the adapter callback we set up in the
     * constructor.
     */
    int ticket = adapter_.make_ticket(adapter_callback_, scheduler,
      std::forward<Args>(args)...);

    /*
     * Enter the active state if we're still here.
     */
    current_scheduler_ = &scheduler;
    current_ticket_ = ticket;
    current_user_callback_ = std::move(packaged_callback);
  }

  /*
   * Cancels any pending user callback; no effect if there is none.
   */
  void reset() noexcept
  {
    if(current_scheduler_ != nullptr)
    {
      /*
       * We are in the active state and have a pending adapter
       * callback; cancel it and revert to the initial state.
       */
      assert(current_ticket_ != -1);
      assert(current_user_callback_ != nullptr);

      adapter_.cancel_ticket(*current_scheduler_, current_ticket_);

      current_scheduler_ = nullptr;
      current_ticket_ = -1;
      current_user_callback_ = nullptr;
    }
  }
      
  ~event_manager_t()
  {
    reset();
  }

private :
  void on_adapter_callback()
  {
    /*
     * This should only happen when we're in the active state.
     */
    assert(current_scheduler_ != nullptr);
    assert(current_ticket_ != -1);
    assert(current_user_callback_ != nullptr);

    /*
     * Because the user callback may call into this event manager, we
     * first revert to the initial state before invoking the user
     * callback, and avoid any access to *this during or after its
     * invocation.
     * By definition, the current ticket is now invalid, so we don't
     * cancel it.
     */
    current_scheduler_ = nullptr;
    current_ticket_ = -1;

    // Move user callback to a safe place (the stack), and go!
    callback_t user_callback = std::move(current_user_callback_);
    user_callback();
  }

private :
  event_adapter_t adapter_;
  callback_t const adapter_callback_;
  scheduler_t* current_scheduler_; // != nullptr when in active state
  int current_ticket_;
  callback_t current_user_callback_;
};
  
} // cuti

#endif
