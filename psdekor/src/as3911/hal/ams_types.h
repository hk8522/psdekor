/*
 *****************************************************************************
 * Copyright by ams AG                                                       *
 * All rights are reserved.                                                  *
 *                                                                           *
 * IMPORTANT - PLEASE READ CAREFULLY BEFORE COPYING, INSTALLING OR USING     *
 * THE SOFTWARE.                                                             *
 *                                                                           *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS       *
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT         *
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS         *
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT  *
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,     *
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT          *
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,     *
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY     *
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT       *
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE     *
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.      *
 *****************************************************************************
 */
/*
 *      PROJECT:   ASxxxx firmware
 *      $Revision: $
 *      LANGUAGE:  ANSI C
 */

/*! \file
 *
 *  \author Unknown
 *  \author Manuel Huber (AVR port)
 *
 *  \brief Basic data types
 *
 *
 *  \note This file has been ported from PIC processor to AVR. Macros from stdint.h
 *        have been used to implement compatible behaviour.
 *
 */

#ifndef AMS_TYPES_H
#define AMS_TYPES_H

#include <asf.h>
#include <stdint.h>

/*!
 * Basic datatypes are mapped to ams datatypes that
 * shall be used in all ams projects.
 */
typedef U8 u8;   /*!< represents an unsigned 8bit-wide type */
typedef S8 s8;     /*!< represents a signed 8bit-wide type */
typedef U16 u16;   /*!< represents an unsigned 16bit-wide type */
typedef S16 s16;     /*!< represents a signed 16bit-wide type */
typedef U32 u32;  /*!< represents an unsigned 32bit-wide type */
typedef U64 u64;/*!< represents an unsigned 64bit-wide type */
typedef S64 s64;  /*!< represents n signed 64bit-wide type */
typedef S32 s32;      /*!< represents a signed 32bit-wide type */
typedef U8 umword; /*!< USE WITH CARE!!! unsigned machine word:
                        8 bit on 8bit machines, 16 bit on 16 bit machines... */
typedef S8 smword; /*!< USE WITH CARE!!! signed machine word:
                         8 bit on 8bit machines, 16 bit on 16 bit machines... */
typedef U16 uint; /*!< type for unsigned integer types,
                            useful as indices for arrays and loop variables */
typedef S16 sint; /*!< type for integer types,
                            useful as indices for arrays and loop variables */

#define U8_C(x)     UINT8_C(x) /*!< Define a constant of type u8 */
#define S8_C(x)     INT8_C(x) /*!< Define a constant of type s8 */
#define U16_C(x)    UINT16_C(x) /*!< Define a constant of type u16 */
#define S16_C(x)    INT16_C(x) /*!< Define a constant of type s16 */
#define U32_C(x)    UINT32_C(x) /*!< Define a constant of type u32 */
#define S32_C(x)    INT32_C(x) /*!< Define a constant of type s32 */
#define U64_C(x)    UINT64_C(x) /*!< Define a constant of type u64 */
#define S64_C(x)    INT64_C(x) /*!< Define a constant of type s64 */
#define UMWORD_C(x) (x) /*!< Define a constant of type umword */
#define MWORD_C(x)  (x) /*!< Define a constant of type mword */

typedef umword bool_t; /*!<
                            represents a boolean type */
#ifndef TRUE
#  define TRUE 1 /*!< used for the #bool_t type */
#endif
#ifndef FALSE
#  define FALSE 0 /*!< used for the #bool_t type */
#endif

#ifndef NULL
#  define NULL (void*)0 /*!< represents a NULL pointer */
#endif

#endif /* AMS_TYPES_H */
