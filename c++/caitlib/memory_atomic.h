/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*****************************************************************************
 *
 *   Non-sleeping memory allocation
 *
 *   Copyright (C) 2006,2007 Nedko Arnaudov <nedko@arnaudov.name>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; version 2 of the License
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *****************************************************************************/

#ifndef MEMORY_ATOMIC_H__7B572547_304D_4597_8808_990BE4476CC3__INCLUDED
#define MEMORY_ATOMIC_H__7B572547_304D_4597_8808_990BE4476CC3__INCLUDED

#ifdef __cplusplus
extern "C" {
#endif
#if 0
} /* Adjust editor indent */
#endif

typedef void * rtsafe_memory_pool_handle;

/* will sleep */
int
rtsafe_memory_pool_create(
  size_t data_size,             /* chunk size */
  size_t min_preallocated,      /* min chunks preallocated */
  size_t max_preallocated,      /* max chunks preallocated */
  rtsafe_memory_pool_handle * pool_ptr);

/* will sleep */
void
rtsafe_memory_pool_destroy(
  rtsafe_memory_pool_handle pool);

/* may sleep */
void
rtsafe_memory_pool_sleepy(
  rtsafe_memory_pool_handle pool);

/* will not sleep, returns NULL if no memory is available */
void *
rtsafe_memory_pool_allocate(
  rtsafe_memory_pool_handle pool);

/* may sleep */
void *
rtsafe_memory_pool_allocate_sleepy(
  rtsafe_memory_pool_handle pool);

/* will not sleep */
void
rtsafe_memory_pool_deallocate(
  rtsafe_memory_pool_handle pool,
  void * data);

typedef void * rtsafe_memory_handle;

/* will sleep */
int
rtsafe_memory_init(
  size_t max_size,
  size_t prealloc_min,
  size_t prealloc_max,
  rtsafe_memory_handle * handle_ptr);

/* will not sleep */
void *
rtsafe_memory_allocate(
  rtsafe_memory_handle handle_ptr,
  size_t size);

void
rtsafe_memory_sleepy(
  rtsafe_memory_handle handle_ptr);

/* will not sleep */
void
rtsafe_memory_deallocate(
  void * data);

void
rtsafe_memory_uninit(
  rtsafe_memory_handle handle_ptr);

#if 0
{ /* Adjust editor indent */
#endif
#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* #ifndef MEMORY_ATOMIC_H__7B572547_304D_4597_8808_990BE4476CC3__INCLUDED */
