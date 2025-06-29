/* ***** BEGIN LICENSE BLOCK *****
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 

/**************************************************************************************
 * Fixed-point MP3 decoder
 * Jon Recker (jrecker@real.com), Ken Cooke (kenc@real.com)
 * June 2003
 *
 * assembly.h - assembly language functions and prototypes for supported platforms
 *
 * - inline rountines with access to 64-bit multiply results 
 * - x86 (_WIN32) and ARM (ARM_ADS, _WIN32_WCE) versions included
 * - some inline functions are mix of asm and C for speed
 * - some functions are in native asm files, so only the prototype is given here
 *
 * MULSHIFT32(x, y)    signed multiply of two 32-bit integers (x and y), returns top 32 bits of 64-bit result
 * FASTABS(x)          branchless absolute value of signed integer x
 * CLZ(x)              count leading zeros in x
 * MADD64(sum, x, y)   (Windows only) sum [64-bit] += x [32-bit] * y [32-bit]
 * SHL64(sum, x, y)    (Windows only) 64-bit left shift using __int64
 * SAR64(sum, x, y)    (Windows only) 64-bit right shift using __int64
 */

#ifndef _ASSEMBLY_H
#define _ASSEMBLY_H

#ifdef __cplusplus
extern "C" {
#endif

#if (defined _WIN32 && !defined _WIN32_WCE) || (defined __WINS__ && defined _SYMBIAN) || defined(_OPENWAVE_SIMULATOR) || defined(WINCE_EMULATOR)    /* Symbian emulator for Ix86 */

#pragma warning( disable : 4035 )	/* complains about inline asm not returning a value */

static __inline int MULSHIFT32(int x, int y)	
{
    __asm {
		mov		eax, x
	    imul	y
	    mov		eax, edx
	}
}

static __inline int FASTABS(int x) 
{
	int sign;

	sign = x >> (sizeof(int) * 8 - 1);
	x ^= sign;
	x -= sign;

	return x;
}

static __inline int CLZ(int x)
{
	int numZeros;

	if (!x)
		return (sizeof(int) * 8);

	numZeros = 0;
	while (!(x & 0x80000000)) {
		numZeros++;
		x <<= 1;
	} 

	return numZeros;
}

/* MADD64, SHL64, SAR64:
 * write in assembly to avoid dependency on run-time lib for 64-bit shifts, muls
 *  (sometimes compiler thunks to function calls instead of code generating)
 * required for Symbian emulator
 */
#ifdef __CW32__
typedef long long Word64;
#else
typedef __int64 Word64;
#endif

static __inline Word64 MADD64(Word64 sum, int x, int y)
{
	unsigned int sumLo = ((unsigned int *)&sum)[0];
	int sumHi = ((int *)&sum)[1];

	__asm {
		mov		eax, x
		imul	y
		add		eax, sumLo
		adc		edx, sumHi
	}

	/* equivalent to return (sum + ((__int64)x * y)); */
}

static __inline Word64 SHL64(Word64 x, int n)
{
	unsigned int xLo = ((unsigned int *)&x)[0];
	int xHi = ((int *)&x)[1];
	unsigned char nb = (unsigned char)n;

	if (n < 32) {
		__asm {
			mov		edx, xHi
			mov		eax, xLo
			mov		cl, nb
			shld    edx, eax, cl
			shl     eax, cl
		}
	} else if (n < 64) {
		/* shl masks cl to 0x1f */
		__asm {
			mov		edx, xLo
			mov		cl, nb
			xor     eax, eax
			shl     edx, cl
		}
	} else {
		__asm {
			xor		edx, edx
			xor		eax, eax
		}
	}
}

static __inline Word64 SAR64(Word64 x, int n)
{
	unsigned int xLo = ((unsigned int *)&x)[0];
	int xHi = ((int *)&x)[1];
	unsigned char nb = (unsigned char)n;

	if (n < 32) {
		__asm {
			mov		edx, xHi
			mov		eax, xLo
			mov		cl, nb
			shrd	eax, edx, cl
			sar		edx, cl
		}
	} else if (n < 64) {
		/* sar masks cl to 0x1f */
		__asm {
			mov		edx, xHi
			mov		eax, xHi
			mov		cl, nb
			sar		edx, 31
			sar		eax, cl
		}
	} else {
		__asm {
			sar		xHi, 31
			mov		eax, xHi
			mov		edx, xHi
		}
	}
}

#elif (defined _WIN32) && (defined _WIN32_WCE)

/* use asm function for now (EVC++ 3.0 does horrible job compiling __int64 version) */
#define MULSHIFT32	xmp3_MULSHIFT32
int MULSHIFT32(int x, int y);

static __inline int FASTABS(int x) 
{
	int sign;

	sign = x >> (sizeof(int) * 8 - 1);
	x ^= sign;
	x -= sign;

	return x;
}

static __inline int CLZ(int x)
{
	int numZeros;

	if (!x)
		return (sizeof(int) * 8);

	numZeros = 0;
	while (!(x & 0x80000000)) {
		numZeros++;
		x <<= 1;
	} 

	return numZeros;
}

#elif defined ARM_ADS

static __inline int MULSHIFT32(int x, int y)
{
    /* important rules for smull RdLo, RdHi, Rm, Rs:
     *     RdHi and Rm can't be the same register
     *     RdLo and Rm can't be the same register
     *     RdHi and RdLo can't be the same register
     * Note: Rs determines early termination (leading sign bits) so if you want to specify
     *   which operand is Rs, put it in the SECOND argument (y)
	 * For inline assembly, x and y are not assumed to be R0, R1 so it shouldn't matter
	 *   which one is returned. (If this were a function call, returning y (R1) would 
	 *   require an extra "mov r0, r1")
     */
    int zlow;
    __asm {
    	smull zlow,y,x,y
   	}

    return y;
}

static __inline int FASTABS(int x) 
{
	int t=0; /*Really is not necessary to initialiaze only to avoid warning*/

	__asm {
		eor	t, x, x, asr #31
		sub	t, t, x, asr #31
	}

	return t;
}

static __inline int CLZ(int x)
{
	int numZeros;

	if (!x)
		return (sizeof(int) * 8);

	numZeros = 0;
	while (!(x & 0x80000000)) {
		numZeros++;
		x <<= 1;
	} 

	return numZeros;
}

#elif defined(__GNUC__) && (defined(ARM) || defined(__ARMEL__)) && (__ARM_ARCH >= 7) && !(__ARM_64BIT_STATE == 1)

static __inline int MULSHIFT32(int x, int y)
{
    /* important rules for smull RdLo, RdHi, Rm, Rs:
     *     RdHi and Rm can't be the same register
     *     RdLo and Rm can't be the same register
     *     RdHi and RdLo can't be the same register
     * Note: Rs determines early termination (leading sign bits) so if you want to specify
     *   which operand is Rs, put it in the SECOND argument (y)
	 * For inline assembly, x and y are not assumed to be R0, R1 so it shouldn't matter
	 *   which one is returned. (If this were a function call, returning y (R1) would
	 *   require an extra "mov r0, r1")
     */
    int zlow;
    __asm__ volatile ("smull %0,%1,%2,%3" : "=&r" (zlow), "=r" (y) : "r" (x), "1" (y)) ;

    return y;
}

static __inline int FASTABS(int x)
{
	int t=0; /*Really is not necessary to initialiaze only to avoid warning*/

	__asm__ volatile (
		"eor %0,%2,%2, asr #31;"
		"sub %0,%1,%2, asr #31;"
		: "=&r" (t)
		: "0" (t), "r" (x)
	 );

	return t;
}

static __inline int CLZ(int x)
{
	int numZeros;

	if (!x)
		return (sizeof(int) * 8);

	numZeros = 0;
	while (!(x & 0x80000000)) {
		numZeros++;
		x <<= 1;
	}

	return numZeros;
}

typedef signed long long int    Word64;  // 64-bit signed integer.
typedef union _U64 {
        Word64 w64;
        struct {
                /* ARM ADS = little endian */
                unsigned int lo32;
                signed int   hi32;
        } r;
} U64;

static __inline Word64 MADD64(Word64 sum64, int x, int y)
{
        U64 u;
        u.w64 = sum64;

        __asm__ volatile ("smlal %0,%1,%2,%3" : "+&r" (u.r.lo32), "+&r" (u.r.hi32) : "r" (x), "r" (y) : "cc");

        return u.w64;
}

__attribute__((__always_inline__)) static __inline Word64 SAR64(Word64 x, int n)
{
  unsigned int xLo = (unsigned int) x;
  int xHi = (int) (x >> 32);
  int nComp = 32-n;
  int tmp;
  // Shortcut: n is always < 32.
  __asm__ __volatile__( "lsl %2, %0, %3\n\t"  // tmp <- xHi<<(32-n)
                        "asr %0, %0, %4\n\t"  // xHi <- xHi>>n
                        "lsr %1, %1, %4\n\t"  // xLo <- xLo>>n
                        "orr  %1, %2\n\t"      // xLo <= xLo || tmp
                        : "+&r" (xHi), "+r" (xLo), "=&r" (tmp)
                        : "r" (nComp), "r" (n) );
  x = xLo | ((Word64)xHi << 32);
  return( x );
}
#elif defined(__GNUC__) && defined(__AVR32_UC__)

typedef signed long long int    Word64;  // 64-bit signed integer.


__attribute__((__always_inline__)) static __inline int MULSHIFT32(int x, int y)
{
    signed long long int s64Tmp;
    __asm__ __volatile__( "muls.d	%0, %1, %2"
                          : "=r" (s64Tmp)
                          : "r" (x), "r" (y) );
		return( s64Tmp >> 32 );
}

__attribute__((__always_inline__)) static __inline int FASTABS(int x)
{
    int tmp;
    __asm__ __volatile__( "abs %0"
                          : "=r" (tmp)
                          : "r" (x) );
    return tmp; 
    
}


__attribute__((__always_inline__))  static __inline int CLZ(int x)
{
    int tmp;
    __asm__ __volatile__( "clz %0,%1"
                          : "=r" (tmp)
                          : "r" (x) );
    return tmp;
}


/* MADD64, SAR64:
 * write in assembly to avoid dependency on run-time lib for 64-bit shifts, muls
 * (sometimes compiler do function calls instead of code generating)
 */
__attribute__((__always_inline__)) static __inline Word64 MADD64(Word64 sum, int x, int y)
{
  __asm__ __volatile__( "macs.d %0, %1, %2"
                        : "+r" (sum)
                        : "r" (x), "r" (y) );
  return( sum );
}


__attribute__((__always_inline__)) static __inline Word64 SAR64(Word64 x, int n)
{
  unsigned int xLo = (unsigned int) x;
  int xHi = (int) (x >> 32);
  int nComp = 32-n;
  int tmp;
  // Shortcut: n is always < 32. 
  __asm__ __volatile__( "lsl %2, %0, %3\n\t"  // tmp <- xHi<<(32-n)
                        "asr %0, %0, %4\n\t"  // xHi <- xHi>>n
                        "lsr %1, %1, %4\n\t"  // xLo <- xLo>>n
                        "or  %1, %2\n\t"      // xLo <= xLo || tmp
                        : "+&r" (xHi), "+r" (xLo), "=&r" (tmp)
                        : "r" (nComp), "r" (n) );
  x = xLo | ((Word64)xHi << 32);
  return( x );
}

#elif (defined(__CORTEX_M) && __CORTEX_M == 0x04U) || defined(__MK66FX1M0__) || defined(__MK64FX512__) || defined(__MK20DX256__)	/* teensy 3.6, 3.5, or 3.1/2 */

/* ARM cortex m4 */

typedef signed long long int    Word64;  // 64-bit signed integer.


static __inline int MULSHIFT32(int x, int y)
{
    /* important rules for smull RdLo, RdHi, Rm, Rs:
     *     RdHi and Rm can't be the same register
     *     RdLo and Rm can't be the same register
     *     RdHi and RdLo can't be the same register
     * Note: Rs determines early termination (leading sign bits) so if you want to specify
     *   which operand is Rs, put it in the SECOND argument (y)
	 * For inline assembly, x and y are not assumed to be R0, R1 so it shouldn't matter
	 *   which one is returned. (If this were a function call, returning y (R1) would
	 *   require an extra "mov r0, r1")
     */
    int zlow;
    __asm__ volatile ("smull %0,%1,%2,%3" : "=&r" (zlow), "=r" (y) : "r" (x), "1" (y)) ;

    return y;
}

static __inline int FASTABS(int x)
{
        int sign;

        sign = x >> (sizeof(int) * 8 - 1);
        x ^= sign;
        x -= sign;

        return x;
}

static __inline int CLZ(int x)
{
#if defined(__MK66FX1M0__) || defined(__MK64FX512__) || defined(__MK20DX256__)	/* teensy 3.6, 3.5, or 3.1/2 */
	return __builtin_clz(x);
#else
	return __CLZ(x);
#endif
}

typedef union _U64 {
        Word64 w64;
        struct {
                /* ARM ADS = little endian */
                unsigned int lo32;
                signed int   hi32;
        } r;
} U64;

static __inline Word64 MADD64(Word64 sum64, int x, int y)
{
        U64 u;
        u.w64 = sum64;
        
        __asm__ volatile ("smlal %0,%1,%2,%3" : "+&r" (u.r.lo32), "+&r" (u.r.hi32) : "r" (x), "r" (y) : "cc");
        
        return u.w64;
}


__attribute__((__always_inline__)) static __inline Word64 SAR64(Word64 x, int n)
{
  unsigned int xLo = (unsigned int) x;
  int xHi = (int) (x >> 32);
  int nComp = 32-n;
  int tmp;
  // Shortcut: n is always < 32. 
  __asm__ __volatile__( "lsl %2, %0, %3\n\t"  // tmp <- xHi<<(32-n)
                        "asr %0, %0, %4\n\t"  // xHi <- xHi>>n
                        "lsr %1, %1, %4\n\t"  // xLo <- xLo>>n
                        "orr  %1, %2\n\t"      // xLo <= xLo || tmp
                        : "+&r" (xHi), "+r" (xLo), "=&r" (tmp)
                        : "r" (nComp), "r" (n) );
  x = xLo | ((Word64)xHi << 32);
  return( x );
}

//END cortex m4


#else

#include <stdint.h>

typedef int64_t Word64;

#if 0
static inline int MULSHIFT32(int x, int y) 
{
    return ((int64_t)x * y) >> 32;
}
#else
__attribute__((__always_inline__)) static inline int MULSHIFT32(int u, int v)
{
    // As specified in "Hackers Delight" by H Warren
    // Compiler created efficient assembler from this C code
    unsigned u0, v0, w0;
    int u1, v1, w1, w2, t;

    u0 = u & 0xFFFF;  u1 = u >> 16;
    v0 = v & 0xFFFF;  v1 = v >> 16;
	// Skip the low multiply as this can only contribute 1 bit difference
	// to the result.
    //w0 = u0*v0;
    t  = u1*v0;// + (w0 >> 16);
    w1 = t & 0xFFFF;
    w2 = t >> 16;
    w1 = u0*v1 + w1;
    return u1*v1 + w2 + (w1 >> 16);
}
#endif

static inline int FASTABS(int x) {
    int sign = x >> 31;
    return (x ^ sign) - sign;
}

static inline int CLZ(int x) {
    return __builtin_clz(x);
}

static inline Word64 MADD64(Word64 sum, int x, int y) {
    return sum + (int64_t)x * y;
}

static inline Word64 SHL64(Word64 x, int n) {
    return x << n;
}

static inline Word64 SAR64(Word64 x, int n) {
    return x >> n;
}

static inline short SAR64_Clip(Word64 x) {
    return SAR64(x, 26);
}

#endif	/* platforms */

#ifdef __cplusplus
}
#endif
#endif /* _ASSEMBLY_H */
