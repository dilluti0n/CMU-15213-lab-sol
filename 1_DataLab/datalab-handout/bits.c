/* 
 * CS:APP Data Lab 
 * 
 * dilluti0n
 * 
 * bits.c - Source file with your solutions to the Lab.
 *          This is the file you will hand in to your instructor.
 *
 * WARNING: Do not include the <stdio.h> header; it confuses the dlc
 * compiler. You can still use printf for debugging without including
 * <stdio.h>, although you might get a compiler warning. In general,
 * it's not good practice to ignore compiler warnings, but in this
 * case it's OK.  
 */

#if 0
/*
 * Instructions to Students:
 *
 * STEP 1: Read the following instructions carefully.
 */

You will provide your solution to the Data Lab by
editing the collection of functions in this source file.

INTEGER CODING RULES:
 
  Replace the "return" statement in each function with one
  or more lines of C code that implements the function. Your code 
  must conform to the following style:
 
  int Funct(arg1, arg2, ...) {
      /* brief description of how your implementation works */
      int var1 = Expr1;
      ...
      int varM = ExprM;

      varJ = ExprJ;
      ...
      varN = ExprN;
      return ExprR;
  }

  Each "Expr" is an expression using ONLY the following:
  1. Integer constants 0 through 255 (0xFF), inclusive. You are
      not allowed to use big constants such as 0xffffffff.
  2. Function arguments and local variables (no global variables).
  3. Unary integer operations ! ~
  4. Binary integer operations & ^ | + << >>
    
  Some of the problems restrict the set of allowed operators even further.
  Each "Expr" may consist of multiple operators. You are not restricted to
  one operator per line.

  You are expressly forbidden to:
  1. Use any control constructs such as if, do, while, for, switch, etc.
  2. Define or use any macros.
  3. Define any additional functions in this file.
  4. Call any functions.
  5. Use any other operations, such as &&, ||, -, or ?:
  6. Use any form of casting.
  7. Use any data type other than int.  This implies that you
     cannot use arrays, structs, or unions.

 
  You may assume that your machine:
  1. Uses 2s complement, 32-bit representations of integers.
  2. Performs right shifts arithmetically.
  3. Has unpredictable behavior when shifting an integer by more
     than the word size.

EXAMPLES OF ACCEPTABLE CODING STYLE:
  /*
   * pow2plus1 - returns 2^x + 1, where 0 <= x <= 31
   */
  int pow2plus1(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     return (1 << x) + 1;
  }

  /*
   * pow2plus4 - returns 2^x + 4, where 0 <= x <= 31
   */
  int pow2plus4(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     int result = (1 << x);
     result += 4;
     return result;
  }

FLOATING POINT CODING RULES

For the problems that require you to implent floating-point operations,
the coding rules are less strict.  You are allowed to use looping and
conditional control.  You are allowed to use both ints and unsigneds.
You can use arbitrary integer and unsigned constants.

You are expressly forbidden to:
  1. Define or use any macros.
  2. Define any additional functions in this file.
  3. Call any functions.
  4. Use any form of casting.
  5. Use any data type other than int or unsigned.  This means that you
     cannot use arrays, structs, or unions.
  6. Use any floating point data types, operations, or constants.


NOTES:
  1. Use the dlc (data lab checker) compiler (described in the handout) to 
     check the legality of your solutions.
  2. Each function has a maximum number of operators (! ~ & ^ | + << >>)
     that you are allowed to use for your implementation of the function. 
     The max operator count is checked by dlc. Note that '=' is not 
     counted; you may use as many of these as you want without penalty.
  3. Use the btest test harness to check your functions for correctness.
  4. Use the BDD checker to formally verify your functions
  5. The maximum number of ops for each function is given in the
     header comment for each function. If there are any inconsistencies 
     between the maximum ops in the writeup and in this file, consider
     this file the authoritative source.

/*
 * STEP 2: Modify the following functions according the coding rules.
 * 
 *   IMPORTANT. TO AVOID GRADING SURPRISES:
 *   1. Use the dlc compiler to check that your solutions conform
 *      to the coding rules.
 *   2. Use the BDD checker to formally verify that your solutions produce 
 *      the correct answers.
 */


#endif

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wparentheses"
#pragma GCC diagnostic ignored "-Wbool-operation"
#pragma GCC diagnostic ignored "-Wint-in-bool-context"
#endif
//1
/* 
 * bitXor - x^y using only ~ and & 
 *   Example: bitXor(4, 5) = 1
 *   Legal ops: ~ &
 *   Max ops: 14
 *   Rating: 1
 */
int bitXor(int x, int y) {
    /*
     * x  y  |  ~(x & ~y)  |   ~(~x & y)  |  ~(x & ~y) & ~(~x & y)  |  x ^ y
     * =======================================================================
     * 1  0  |      0      |       1      |           0             |    1
     * 0  1  |      1      |       0      |           0             |    1
     * 1  1  |      1      |       1      |           1             |    0
     * 0  0  |      1      |       1      |           1             |    0    
     * =======================================================================
     * this implies
     * x | y = ~(~x & ~y) (De Morgan)
     * and
     * x ^ y = ~((x & ~y) | (~x & y))
     *       = ~(~(x & ~y) & ~(~x & y)).
     * furthermore,
     * x ^ y = ~(x & y) & (x | y)
     *       = ~(x & y) & ~(~x & ~y)
     * could be possablie answaer too.
     */
    return ~(x & y) & ~(~x & ~y);
}
/* 
 * tmin - return minimum two's complement integer 
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 4
 *   Rating: 1
 */
int tmin(void) {
    /* int is 32 bit, which implies tmin is 0x80000000, which is 1 << 31 */
    return 1 << 31;
}
//2
/*
 * isTmax - returns 1 if x is the maximum, two's complement number,
 *     and 0 otherwise 
 *   Legal ops: ! ~ & ^ | +
 *   Max ops: 10
 *   Rating: 2
 */
int isTmax(int x) {
    /*
     * Tmax = ~tmin,
     * ~x + 1 = x implies x = 0 or x = tmin.
     * thus x + 1 = ~x only if x = -1 or x = Tmax.
     */
    return !((x + 1) ^ ~x) & !!~x;
}
/* 
 * allOddBits - return 1 if all odd-numbered bits in word set to 1
 *   Examples allOddBits(0xFFFFFFFD) = 0, allOddBits(0xAAAAAAAA) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 2
 */
int allOddBits(int x) {
    /*
     * x  y  |  x & ~y
     * =================
     * 0  1  |     0
     * 1  0  |     1
     * 0  0  |     0
     * 1  1  |     0
     * =================
     * if x = 0xAAAAAAAA, x & ~y yields non-zero only if there is zero bit on y's 
     * odd bits, the case which is intended to return zero on this function.
     * Single byte of all odd-numbered bits set to 1 and all 
     * even-numbered bits set to 0 is 0xAA (1010 1010).
     */
    int mask = 0xAA << 8 | 0xAA; /* 0xAAAA, two bytes */
    mask = mask << 16 | mask; /* 0xAAAAAAAA, four bytes */
    return !(mask & ~x);
}
/* 
 * negate - return -x 
 *   Example: negate(1) = -1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 5
 *   Rating: 2
 */
int negate(int x) {
    return ~x + 1; 
    /* note: negating tmin will lead buffer overflow. negate(tmin) = tmin */
}
//3
/* 
 * isAsciiDigit - return 1 if 0x30 <= x <= 0x39 (ASCII codes for characters '0' to '9')
 *   Example: isAsciiDigit(0x35) = 1.
 *            isAsciiDigit(0x3a) = 0.
 *            isAsciiDigit(0x05) = 0.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 3
 */
int isAsciiDigit(int x) {
    /*
     * 0x30 (0011 0000), 0x39 (0011 1001).
     * 0x30 <= x <= 0x39 if and only if x >> 3 = 6(0110) or x >> 1 = 0x1c(0001 1100).
     * and x ^ y = 0 implies x = y.
     */
    return !(x >> 3 ^ 6) | !(x >> 1 ^ 0x1c);
}
/* 
 * conditional - same as x ? y : z 
 *   Example: conditional(2,4,5) = 4
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 16
 *   Rating: 3
 */
int conditional(int x, int y, int z) {
    /*
     * This function intended to be returning z if x is zero, and y if not.
     * if x is zero    ; !x = 1, ~!x + 1 = 0xFFFFFFFF, (~!x + 1) & z = z.
     * if x is non-zero; !x = 0, ~!x + 1 = 0,          (~!x + 1) & z = 0.
     * we can do it the same way for !x and y. then the pair become
     * ((~!!x + 1) & y, (~!x + 1) & z) = (y, 0) if x is non-zero
     *                                 = (0, z) if x = 0.
     * Thus the value we need to return is ((~!!x + 1) & y) | ((~!x + 1) & z).
     */
    return ((~!!x + 1) & y) | ((~!x + 1) & z);
}
/* 
 * isLessOrEqual - if x <= y  then return 1, else return 0 
 *   Example: isLessOrEqual(4,5) = 1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
int isLessOrEqual(int x, int y) {
    int signX = x & 1 << 31;
    int signY = y & 1 << 31;
    int indicator = signX ^ signY; /* 0 if same sign, tmin if diffrent sign */
    int diff = y + ~x + 1; /* diff = y - x */
    diff &= 1 << 31; /* if y - x < 0, diff = tmin, y - x >= 0, diff = 0 */
    return (!diff & !indicator) | (!signY & !!indicator);
}
//4
/* 
 * logicalNeg - implement the ! operator, using all of 
 *              the legal operators except !
 *   Examples: logicalNeg(3) = 0, logicalNeg(0) = 1
 *   Legal ops: ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 4 
 */
int logicalNeg(int x) {
    /* 
     * given x, sign bit of -x is same only if x = 0 or x = tmin. 
     * if x = 0, sign bit is both 0 and for x = tmin, it is both 1.
     */
    int signX = x & (1 << 31);
    int signNegX = (~x + 1) & (1 << 31);
    return ~(~(signX | signNegX) >> 31) + 1; /* assume arithmetic right shift */
}
/* howManyBits - return the minimum number of bits required to represent x in
 *             two's complement
 *  Examples: howManyBits(12) = 5
 *            howManyBits(298) = 10
 *            howManyBits(-5) = 4
 *            howManyBits(0)  = 1
 *            howManyBits(-1) = 1
 *            howManyBits(0x80000000) = 32
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 90
 *  Rating: 4
 */
int howManyBits(int x) {
    /*
     * If the given integer is unsigned, then the number of bits required for 
     * the unsigned bit representation is simply the position of the 
     * leftmost '1' bit.
     * 
     * However, since we are dealing with the two's complement representation, in the case 
     * where x is positive (i.e., the leading bit is 0), we need to find the position 
     * of the leftmost '1' bit just like in the unsigned case, and add 1 for the 
     * sign bit. In the case where x is negative (i.e., the leading bit is 1), 
     * we need to find the position of the leftmost '0' bit and add 1.
     * 
     * It can be done same for all signed values, if x is negative, complementing x and then 
     * adding 1 to the position of the leading-1. 
     * 
     * To efficiently find the position of the leading-1, We will use a bit version of binary search.
     */
    int signX, mask_16;
    int Bool_16, Bool_8, Bool_4, Bool_2, Bool_1;
    signX = ~((x >> 31) & 1) + 1; /* -1 if x is negative, 0 if x is positive. */
    x = signX & ~x | ~signX & x; /* if x is negative, complement it. */
    mask_16 = 0xFF << 8 | 0xFF; /* 0xFFFF */
    Bool_16 = ~!!(mask_16 & x >> 16) + 1; /* -1 if leading-1 is in upper 16 bits, 0 if not. */
    x = Bool_16 & (x >> 16) | ~Bool_16 & x; /* now x is 16-bit */
    /* do same to 8-bits */
    Bool_8 = ~!!(0xFF & x >> 8) + 1;
    x = Bool_8 & (x >> 8) | ~Bool_8 & x; /* now x is 8-bit */
    /* do same to 4-bits */
    Bool_4 = ~!!(0xF & x >> 4) + 1;
    x = Bool_4 & (x >> 4) | ~Bool_4 & x; /* now x is 4-bit */
    /* do same to 2-bits */
    Bool_2 = ~!!(3 & x >> 2) + 1;
    x = Bool_2 & (x >> 2) | ~Bool_2 & x; /* now x is 2-bit */
    /* do same to 1-bit */
    Bool_1 = ~!!(1 & x >> 1) + 1; /* now we have the position of leading-1 bit. */
    x = Bool_1 & (x >> 1) | ~Bool_1 & x; /* if there was no leading-1 bit, it will be zero. else 1. */
    return (Bool_16 & 16) + (Bool_8 & 8) + (Bool_4 & 4) + (Bool_2 & 2) + (Bool_1 & 1) + x + 1;
}
//float
/* 
 * float_twice - Return bit-level equivalent of expression 2*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned float_twice(unsigned uf) {
    unsigned exp = uf >> 23 & 0xFF;
    unsigned frac = uf & -1U >> 9;
    if (!~(exp | -1U << 8)) /* uf is inf or NaN */
        return uf;
    if (!exp) { /* exp is zero */
        if (frac & 1 << 22) /* leading of frac is 1 */
            exp = 1;
        frac <<= 1;
    } else if (!~(((exp += 1) | - 1U << 8))) { /* exp is non-zero */
        frac = 0;
    }
    return uf & 1 << 31 | exp << 23 | frac & -1U >> 9;
}
/* 
 * float_i2f - Return bit-level equivalent of expression (float) x
 *   Result is returned as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point values.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned float_i2f(int x) {
    unsigned ux;
    unsigned exp = 0, frac, sign = x & 0x80000000;
    unsigned grd_b, rnd_b, stky_b;
    if (sign) /* if x is negative */
        x = ~x + 1; /* negate x. */
    ux = (unsigned) x;
    while (ux) {
        exp += 1;
        ux >>= 1; /* logical shift */
    } /* exp: maximum bit to represent ux */
    if (!exp) /* if exp is zero */
        return 0; /* x is zero. */
    if (exp > 24) { /* need to round. */
        frac = (unsigned) x >> (exp - 24); /* logical shift */
        grd_b = frac & 1;
        rnd_b = x >> (exp - 25) & 1;
        stky_b = !!(x << (57 - exp));
        frac += rnd_b & (stky_b | grd_b); /* round frac with IEEE 754 rounding rules. */
        if (frac & 0x1000000) /* if 25th bit of frac is non-zero, */
            exp += 1; /* round overflowed. */
    } else { /* 0 < exp <= 24 */
        frac = (unsigned) x << (24 - exp);
    }
    frac &= 0x7fffff; /* 0x7fffff: mask for frac bits. */
    exp = (exp + 126) << 23; /* exp: exponential part of result float */
    return sign | exp | frac;
}
/* 
 * float_f2i - Return bit-level equivalent of expression (int) f
 *   for floating point argument f.
 *   Argument is passed as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point value.
 *   Anything out of range (including NaN and infinity) should return
 *   0x80000000u.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
int float_f2i(unsigned uf) {
    unsigned sign = uf & 0x80000000;
    unsigned exp = uf & 0x7f800000;
    unsigned frac = uf & 0x7fffff; /* parse each parts of uf */
    int E;
    if (!exp) /* denomalized. */
        return 0;
    frac |= 0x800000; /* set 24th bit of frac 1. */
    /* now frac is 1.xxx * 2^23 unsigned. */
    E = (exp >> 23) - 127;
    if (E > 30) /* uf is inf or NaN when E is 128, which is included in this case. */
        return 0x80000000;
    if (E > 23) /* E = 24, 25, ... , 30 */
        frac <<= (E - 23);
    else if (E >= 0) /* E = 0, 1, ..., 23 */
        frac >>= (23 - E); /* truncate the exceed bits. (not round!) */
    else
        frac = 0;
    if (sign) /* if sign is negative */
        frac = ~frac + 1; /* negate result. */
    return frac;
}
