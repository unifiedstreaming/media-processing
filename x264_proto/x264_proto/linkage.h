/*
 * Copyright (C) 2022 CodeShop B.V.
 *
 * This file is part of the x264 service protocol library.
 *
 * The x264 service protocol library is free software: you can
 * redistribute it and/or modify it under the terms of version 2.1 of
 * the GNU Lesser General Public License as published by the Free
 * Software Foundation.
 *
 * The x264 service protocol library is distributed in the hope that
 * it will be useful, but WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See version 2.1 of the GNU Lesser General Public License
 * for more details.
 * 
 * You should have received a copy of version 2.1 of the GNU Lesser
 * General Public License along with the x264 service protocol
 * library.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef X264_PROTO_LINKAGE_H_
#define X264_PROTO_LINKAGE_H_

#ifdef _WIN32
#define X264_PROTO_EXPORT __declspec(dllexport)
#define X264_PROTO_IMPORT __declspec(dllimport)
#else
#define X264_PROTO_EXPORT __attribute__ ((visibility ("default")))
#define X264_PROTO_IMPORT __attribute__ ((visibility ("default")))
#endif

#ifdef BUILDING_X264_PROTO
#define X264_PROTO_ABI X264_PROTO_EXPORT
#else
#define X264_PROTO_ABI X264_PROTO_IMPORT
#endif

#endif // X264_PROTO_LINKAGE_H_
