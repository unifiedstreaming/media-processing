/*
 * Copyright (C) 2021-2022 CodeShop B.V.
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

#ifndef CUTI_LINKAGE_H_
#define CUTI_LINKAGE_H_

#ifdef _WIN32
#define CUTI_EXPORT __declspec(dllexport)
#define CUTI_IMPORT __declspec(dllimport)
#else
#define CUTI_EXPORT __attribute__ ((visibility ("default")))
#define CUTI_IMPORT __attribute__ ((visibility ("default")))
#endif

#ifdef BUILDING_CUTI
#define CUTI_ABI CUTI_EXPORT
#else
#define CUTI_ABI CUTI_IMPORT
#endif

#endif // CUTI_LINKAGE_H

