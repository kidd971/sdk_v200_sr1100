/** @file sr1100_reg.h
 *  @brief SR1100 registers map main include.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is confidential and proprietary.
 *  @author    SPARK FW Team.
 */
#ifndef SR1100_REG_H_
#define SR1100_REG_H_

/* These define the offsets for a little-endian processor */
#define FIRST_BYTE_OFFSET 0
#define SECOND_BYTE_OFFSET 8

/* MACROS *********************************************************************/
/*! Register field single bit mask */
#define BIT(n)              (1 << (n))
/*! Register field single bit mask */
#define BIT16_1(n)          (1 << ((n) + FIRST_BYTE_OFFSET))
/*! Register field single bit mask */
#define BIT16_2(n)          (1 << ((n) + SECOND_BYTE_OFFSET))
/*! Calculates leftshift for given mask (e.g BITS2SHIFT(0x30) = 16, where log2(16) = 4, to shift 4). */
#define BITS2SHIFT(mask)    ((mask) & -(mask))
/*! Register field multibit mask, 8-bit.  */
#define BITS8(b, a)         ((0xff >> (7 - (b))) & ~((1U << (a)) - 1))
/*! Register field multibit mask, 16-bit */
#define BITS16(b, a)        ((0xffff >> (15 - (b))) & ~((1U << (a)) - 1))
/*! Register field multibit mask, 16-bit */
#define BITS16_1(b, a)      ((0xffff >> (15 - ((b) + FIRST_BYTE_OFFSET))) & ~((1U << ((a) + FIRST_BYTE_OFFSET)) - 1))
/*! Register field multibit mask, 16-bit */
#define BITS16_2(b, a)      ((0xffff >> (15 - ((b) + SECOND_BYTE_OFFSET))) & ~((1U << ((a) + SECOND_BYTE_OFFSET)) - 1))
/*! Register field multibit mask, 24-bit */
#define BITS24(b, a)        ((0xffffff >> (23 - (b))) & ~((1U << (a)) - 1))
/*! Register field multibit mask, 32-bit */
#define BITS32(b, a)        ((0xffffffff >> (31 - (b))) & ~((1U << (a)) - 1))
/*! Returns values written in mask */
#define MASK2VAL(val, mask) (((val) & (mask)) / BITS2SHIFT(mask))
/*! Returns a value within the given mask */
#define MOV2MASK(val, mask) (((val) * BITS2SHIFT(mask)) & (mask))

/* CONSTANTS ******************************************************************/
#define REG_READ_BURST   BIT(7)
#define REG_WRITE        BIT(6)
#define REG_WRITE_BURST (BIT(7) | REG_WRITE)

/* INCLUDES *******************************************************************/
#include "sr1100_v3_reg.h"

/*! Returns true if register at the provided adress is 16 bits, 0 otherwise */
#define REG_IS_16_BITS(x) ((x) < REG8_FIFOS ? 1 : 0)

#endif /* SR1100_REG_H_ */
