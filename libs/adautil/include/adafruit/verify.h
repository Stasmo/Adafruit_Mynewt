/******************************************************************************/
/*!
    @file     verify.h
    @author   hathach

    @section LICENSE

    Software License Agreement (BSD License)

    Copyright (c) 2013, K. Townsend (microBuilder.eu)
    Copyright (c) 2015, Adafruit Industries (adafruit.com)
    Copyright (c) 2016, hathach (tinyusb.org)

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
    INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    INCLUDING NEGLIGENCE OR OTHERWISE ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/******************************************************************************/

#ifndef _VERIFY_H_
#define _VERIFY_H_

#include "compiler_macro.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define RETURN_VOID
#define err_t int

//--------------------------------------------------------------------+
// Compile-time Assert
//--------------------------------------------------------------------+
#if defined __COUNTER__ && __COUNTER__ != __COUNTER__
  #define _VERIFY_COUNTER __COUNTER__
#else
  #define _VERIFY_COUNTER __LINE__
#endif

#define VERIFY_STATIC(const_expr, message) enum { XSTRING_CONCAT_(static_verify_, _VERIFY_COUNTER) = 1/(!!(const_expr)) }

//--------------------------------------------------------------------+
// VERIFY Helper
//--------------------------------------------------------------------+
#if CFG_DEBUG >= 1
//  #define VERIFY_MESS(format, ...) printf("[%08ld] %s: %d: verify failed\n", get_millis(), __func__, __LINE__)
  #define VERIFY_MESS(_status)   printf("%s: %d: verify failed status = %d\n", __func__, __LINE__, _status);
#else
  #define VERIFY_MESS(_status)
#endif


#define VERIFY_DEFINE(_status, _error) \
    if ( 0 != _status ) {           \
      VERIFY_MESS(_status) /*;*/\
      return _error;               \
    }

/*------------------------------------------------------------------*/
/* VERIFY STATUS
 * - VERIFY_GETARGS, VERIFY_1ARGS, VERIFY_2ARGS are helper to implement
 * optional parameter for VERIFY_STATUS
 *------------------------------------------------------------------*/
#define VERIFY_GETARGS(arg1, arg2, arg3, ...)  arg3

#define VERIFY_1ARGS(sts)             \
    do {                              \
      err_t _status = (err_t)(sts);   \
      VERIFY_DEFINE(_status, _status);\
    } while(0)

#define VERIFY_2ARGS(sts, _error)     \
    do {                              \
      err_t _status = (err_t)(sts);   \
      VERIFY_DEFINE(_status, _error);\
    } while(0)

/**
 * Check if status is success (zero), otherwise return
 * - status value if called with 1 parameter e.g VERIFY_STATUS(status)
 * - 2 parameter if called with 2 parameters e.g VERIFY_STATUS(status, errorcode)
 */
#define VERIFY_STATUS(...)  VERIFY_GETARGS(__VA_ARGS__, VERIFY_2ARGS, VERIFY_1ARGS)(__VA_ARGS__)


#ifdef __cplusplus
}
#endif

#endif /* _VERIFY_H_ */
