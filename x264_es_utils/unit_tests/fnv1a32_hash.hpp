/*
 * Copyright (C) 2024 CodeShop B.V.
 *
 * This file is part of the x264_es_utils library.
 *
 * The x264_es_utils library is free software: you can redistribute it
 * and/or modify it under the terms of version 2 of the GNU General
 * Public License as published by the Free Software Foundation.
 *
 * The x264_es_utils library is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See version 2 of the GNU General Public License for more details.
 *
 * You should have received a copy of version 2 of the GNU General
 * Public License along with the x264_es_utils library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef FNV1A32_HASH_HPP_
#define FNV1A32_HASH_HPP_

namespace fnv1a32
{

// FNV-1a offset basis
constexpr uint32_t init = 2166136261;

// 32 bit magic FNV-1 prime (seed<<1)+(seed<<4)+(seed<<7)+(seed<<8)+(seed<<24)
constexpr uint32_t prime = 16777619;

struct hash_t
{
  hash_t()
  : seed_(init)
  {
  }

  hash_t(hash_t const&) = delete;
  hash_t& operator=(hash_t const&) = delete;

  void update(uint8_t const* first, uint8_t const* last)
  {
    while(first != last)
    {
      seed_ ^= static_cast<uint32_t>(*first++);
      seed_ *= prime;
    }
  }

  uint32_t final() const
  {
    return seed_;
  }

private:
  uint32_t seed_;
};

uint32_t hash(uint8_t const* first, size_t size)
{
  hash_t hash;
  hash.update(first, first + size);
  return hash.final();
}

} // fnv1a32

#endif
