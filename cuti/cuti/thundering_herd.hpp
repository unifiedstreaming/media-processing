/*
 * Copyright (C) 2025 CodeShop B.V.
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

#ifndef CUTI_THUNDERING_HERD_HPP_
#define CUTI_THUNDERING_HERD_HPP_

#include "linkage.h"

#include <condition_variable>
#include <mutex>

namespace cuti
{

struct CUTI_ABI thundering_herd_fence_t
{
  explicit thundering_herd_fence_t(unsigned int n_threads)
  : mutex_()
  , open_()
  , countdown_(n_threads)
  { }

  thundering_herd_fence_t(thundering_herd_fence_t const&) = delete;
  thundering_herd_fence_t& operator=(thundering_herd_fence_t const&) = delete;

  void pass()
  {
    bool did_open = false;
    {
      std::unique_lock<std::mutex> lock(mutex_);
      if(countdown_ != 0)
      {
        --countdown_;
        if(countdown_ == 0)
        {
          did_open = true;
        }
        else while(countdown_ != 0)
        {
          open_.wait(lock);
        }
      }
    }

    if(did_open)
    {
      open_.notify_all();
    }
  }

private :
  std::mutex mutex_;
  std::condition_variable open_;
  unsigned int countdown_;
};

} // cuti

#endif // CUTI_THUNDERING_HERD_HPP_
