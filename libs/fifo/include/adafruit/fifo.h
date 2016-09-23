/******************************************************************************/
/*!
    @file     fifo.h
    @author   hathach (tinyusb.org)

    @section LICENSE

    Software License Agreement (BSD License)

    Copyright (c) 2012, K. Townsend (microBuilder.eu)
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
    1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    3. Neither the name of the copyright holders nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/******************************************************************************/
#ifndef __FIFO_H__
#define __FIFO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "adafruit/adafruit_util.h"

#if 0
#include "wiced_rtos.h"
#define ff_mutex_t           wiced_semaphore_t
#define ff_mutex_lock(_f)     if ((_f)->mutex) wiced_rtos_lock_mutex((_f)->mutex)
#define ff_mutex_unlock(_f)   if ((_f)->mutex) wiced_rtos_unlock_mutex((_f)->mutex)
#else
#define ff_mutex_lock(x)
#define ff_mutex_unlock(x)
#endif

typedef struct _fifo_t
{
           uint8_t* const buffer    ; ///< buffer pointer
           uint16_t const depth     ; ///< max items
           uint16_t const item_size ; ///< size of each item
  volatile uint16_t count           ; ///< number of items in queue
  volatile uint16_t wr_idx          ; ///< write pointer
  volatile uint16_t rd_idx          ; ///< read pointer
  bool        const overwritable;

  #ifdef ff_mutex_t
  ff_mutex_t * const mutex;
  #endif
} fifo_t;

#define FIFO_DEF(name, ff_depth, type, is_overwritable /*,_mutex*/)\
  type name##_buffer[ff_depth];\
  fifo_t * const name = &((fifo_t) {\
      .buffer       = (uint8_t*) name##_buffer,\
      .depth        = ff_depth,\
      .item_size    = sizeof(type),\
      .overwritable = is_overwritable,\
      /*.mutex        = _mutex,*/\
  })

void fifo_clear(fifo_t *f);
bool fifo_write(fifo_t* f, void const * p_data);
bool fifo_read(fifo_t* f, void * p_buffer);
bool fifo_peek_at(fifo_t* f, uint16_t position, void * p_buffer);

static inline bool fifo_peek(fifo_t* f, void * p_buffer) ATTR_ALWAYS_INLINE;
static inline bool fifo_peek(fifo_t* f, void * p_buffer)
{
  return fifo_peek_at(f, 0, p_buffer);
}

uint16_t fifo_read_n (fifo_t* f, void * p_buffer, uint16_t count);
uint16_t fifo_write_n(fifo_t* f, void const * p_data, uint16_t count);

static inline bool fifo_empty(fifo_t* f) ATTR_ALWAYS_INLINE;
static inline bool fifo_empty(fifo_t* f)
{
  return (f->count == 0);
}

static inline bool fifo_full(fifo_t* f) ATTR_ALWAYS_INLINE;
static inline bool fifo_full(fifo_t* f)
{
  return (f->count == f->depth);
}

static inline uint16_t fifo_get_count(fifo_t* f) ATTR_ALWAYS_INLINE;
static inline uint16_t fifo_get_count(fifo_t* f)
{
  return f->count;
}

static inline uint16_t fifo_get_depth(fifo_t* f) ATTR_ALWAYS_INLINE;
static inline uint16_t fifo_get_depth(fifo_t* f)
{
  return f->depth;
}


#ifdef __cplusplus
}
#endif 

#endif
