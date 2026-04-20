/** @file sr_bit_macros.h
 *  @brief Macros for bit-level register operations of the SR1x00 radio.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is confidential and proprietary.
 *  @author    SPARK FW Team.
 */
#ifndef SR_BIT_MACROS_H_
#define SR_BIT_MACROS_H_

/*! These define the offsets for a little-endian processor. */
#define FIRST_BYTE_OFFSET  0
#define SECOND_BYTE_OFFSET 8

/* MACROS *********************************************************************/
/*! Register field single bit mask. */
#define BIT(n) (1 << (n))
/*! Register field multibit mask, 8-bit.  */
#define BITS8(b, a) ((0xff >> (7 - (b))) & ~((1U << (a)) - 1))
/*! Register field single bit mask. */
#define BIT16_1(n) (1 << ((n) + FIRST_BYTE_OFFSET))
/*! Register field single bit mask. */
#define BIT16_2(n) (1 << ((n) + SECOND_BYTE_OFFSET))
/*! Register field multibit mask, 16-bit. */
#define BITS16(b, a) ((0xffff >> (15 - (b))) & ~((1U << (a)) - 1))
/*! Register field multibit mask, 16-bit. */
#define BITS16_1(b, a) ((0xffff >> (15 - ((b) + FIRST_BYTE_OFFSET))) & ~((1U << ((a) + FIRST_BYTE_OFFSET)) - 1))
/*! Register field multibit mask, 16-bit. */
#define BITS16_2(b, a) ((0xffff >> (15 - ((b) + SECOND_BYTE_OFFSET))) & ~((1U << ((a) + SECOND_BYTE_OFFSET)) - 1))
/*! Register field multibit mask, 24-bit. */
#define BITS24(b, a) ((0xffffff >> (23 - (b))) & ~((1U << (a)) - 1))
/*! Register field multibit mask, 32-bit. */
#define BITS32(b, a) ((0xffffffff >> (31 - (b))) & ~((1U << (a)) - 1))
/*! Calculates leftshift for given mask (e.g BITS2SHIFT(0x30) = 16, where log2(16) = 4, to shift 4). */
#define BITS2SHIFT(mask) ((mask) & -(mask))
/*! Returns values written in mask. */
#define MASK2VAL(val, mask) (((val) & (mask)) / BITS2SHIFT(mask))
/*! Returns a value within the given mask. */
#define MOV2MASK(val, mask) (((val) * BITS2SHIFT(mask)) & (mask))
/*! Return the max value of a given mask. */
#define MASK_MAX_VALUE(mask) ((mask) / BITS2SHIFT(mask))

#endif /* SR_BIT_MACROS_H_ */
