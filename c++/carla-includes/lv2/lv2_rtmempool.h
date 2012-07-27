/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*****************************************************************************
 *
 *  This work is in public domain.
 *
 *  This file is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  If you have questions, contact Nedko Arnaudov <nedko@arnaudov.name> or
 *  ask in #lad channel, FreeNode IRC network.
 *
 *****************************************************************************/

/**
 * @file lv2_rtmempool.h
 * @brief LV2 realtime safe memory pool extension definition
 *
 */

#ifndef LV2_RTMEMPOOL_H__8914012A_720D_4EAC_B0DB_6F93F2B47975__INCLUDED
#define LV2_RTMEMPOOL_H__8914012A_720D_4EAC_B0DB_6F93F2B47975__INCLUDED

#define LV2_RTSAFE_MEMORY_POOL_URI "http://home.gna.org/lv2dynparam/rtmempool/v1"

#ifdef __cplusplus
extern "C" {
#endif
#if 0
} /* Adjust editor indent */
#endif

/** opaque handle to memory pool */
typedef struct { /** fake */ int unused; } * lv2_rtsafe_memory_pool_handle;

/** max size of memory pool name, in chars, including terminating zero char */
#define LV2_RTSAFE_MEMORY_POOL_NAME_MAX 128

/** structure, pointer to which is to be supplied as @c data member of ::LV2_Feature */
typedef struct
{
  /**
   * This function is called when plugin wants to create memory pool
   *
   * <b>may/will sleep</b>
   *
   * @param pool_name pool name, for debug purposes, max RTSAFE_MEMORY_POOL_NAME_MAX chars, including terminating zero char. May be NULL.
   * @param data_size memory chunk size
   * @param min_preallocated min chunks preallocated
   * @param max_preallocated chunks preallocated
   * @param pool_ptr pointer to variable receiving handle to newly created pool object
   *
   * @return Success status
   * @retval Non-zero - success
   * @retval Zero - error
   */
  unsigned char
  (*create)(
    const char * pool_name,
    size_t data_size,             /* chunk size */
    size_t min_preallocated,      /* min chunks preallocated */
    size_t max_preallocated,      /* max chunks preallocated */
    lv2_rtsafe_memory_pool_handle * pool_ptr);

  /**
   * This function is called when plugin wants to destroy previously created memory pool
   *
   * <b>may/will sleep</b>
   *
   * @param pool handle to pool object
   */
  void
  (*destroy)(
    lv2_rtsafe_memory_pool_handle pool);

  /**
   * This function is called when plugin wants to allocate memory in context where sleeping is not allowed
   *
   * <b>will not sleep</b>
   *
   * @param pool handle to pool object
   *
   * @return Pointer to allocated memory or NULL if memory no memory is available
   */
  void *
  (*allocate_atomic)(
    lv2_rtsafe_memory_pool_handle pool);

  /**
   * This function is called when plugin wants to allocate memory in context where sleeping is allowed
   *
   * <b>may/will sleep</b>
   *
   * @param pool handle to pool object
   *
   * @return Pointer to allocated memory or NULL if memory no memory is available (should not happen under normal conditions)
   */
  void *
  (*allocate_sleepy)(
    lv2_rtsafe_memory_pool_handle pool);

  /**
   * This function is called when plugin wants to deallocate previously allocated memory
   *
   * <b>will not sleep</b>
   *
   * @param pool handle to pool object
   * @param memory_ptr pointer to previously allocated memory chunk
   */
  /* will not sleep */
  void
  (*deallocate)(
    lv2_rtsafe_memory_pool_handle pool,
    void * memory_ptr);
} lv2_rtsafe_memory_pool_provider;

#if 0
{ /* Adjust editor indent */
#endif
#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* #ifndef LV2_RTMEMPOOL_H__8914012A_720D_4EAC_B0DB_6F93F2B47975__INCLUDED */
