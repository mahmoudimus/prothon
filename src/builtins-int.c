/* ====================================================================
 * The Prothon License Agreement, Version 1.1
 *
 * Copyright (c) 2004 Hahn Creative Applications, http://hahnca.com.
 * All rights reserved. 
 *
 * 1. This LICENSE AGREEMENT is between Hahn Creative Applications ("HCA"),
 * and the Individual or Organization ("Licensee") accessing and otherwise
 * using Prothon software in source or binary form and its associated
 * documentation.
 * 
 * 2. Subject to the terms and conditions of this License Agreement, HCA
 * hereby grants Licensee a nonexclusive, royalty-free, world-wide license
 * to reproduce, analyze, test, perform and/or display publicly, prepare
 * derivative works, distribute, and otherwise use Prothon alone or in any
 * derivative version, provided, however, that HCA's License Agreement and
 * HCA's notice of copyright, i.e., "Copyright (c) 2004 Hahn Creative
 * Applications; All Rights Reserved" are retained in Prothon alone or
 * in any derivative version prepared by Licensee.
 * 
 * 3. In the event Licensee prepares a derivative work that is based on or
 * incorporates Prothon or any part thereof, and wants to make the
 * derivative work available to others as provided herein, then Licensee
 * hereby agrees to include in any such work a brief summary of the
 * changes made to Prothon.
 * 
 * 4. HCA is making Prothon available to Licensee on an "AS IS" basis.
 * HCA MAKES NO REPRESENTATIONS OR WARRANTIES, EXPRESS OR IMPLIED. BY WAY
 * OF EXAMPLE, BUT NOT LIMITATION, HCA MAKES NO AND DISCLAIMS ANY
 * REPRESENTATION OR WARRANTY OF MERCHANTABILITY OR FITNESS FOR ANY
 * PARTICULAR PURPOSE OR THAT THE USE OF PROTHON WILL NOT INFRINGE ANY
 * THIRD PARTY RIGHTS.
 * 
 * 5. HCA SHALL NOT BE LIABLE TO LICENSEE OR ANY OTHER USERS OF PROTHON
 * FOR ANY INCIDENTAL, SPECIAL, OR CONSEQUENTIAL DAMAGES OR LOSS AS A
 * RESULT OF MODIFYING, DISTRIBUTING, OR OTHERWISE USING PROTHON, OR ANY
 * DERIVATIVE THEREOF, EVEN IF ADVISED OF THE POSSIBILITY THEREOF.
 * 
 * 6. This License Agreement will automatically terminate upon a material
 * breach of its terms and conditions.
 * 
 * 7. Nothing in this License Agreement shall be deemed to create any
 * relationship of agency, partnership, or joint venture between HCA and
 * Licensee. This License Agreement does not grant permission to use HCA
 * trademarks or trade name in a trademark sense to endorse or promote
 * products or services of Licensee, or any third party.
 * 
 * 8. By copying, installing or otherwise using Prothon, Licensee agrees
 * to be bound by the terms and conditions of this License Agreement.
 * ====================================================================
 */

// builtins-int.c

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include <apr_strings.h>

#include <prothon/prothon.h>
#include "dict.h"
#include "parser.h"
#include "object.h"
#include <prothon/prothon_dll.h>

/* includes (arbitrary precision) integer object implementation */
// copied from python/python/dist/src/Objects/longobject.c
// and heavily modified for use in Prothon

/* For long multiplication, use the O(N**2) school algorithm unless
 * both operands contain more than KARATSUBA_CUTOFF digits (this
 * being an internal long digit, in base BASE).
 */
#define KARATSUBA_CUTOFF 35

#ifndef INT_MAX
#define INT_MAX       2147483647    /* maximum (signed) int value */
#endif
#define Py_IS_INFINITY(X) ((X) && (X)*0.5 == (X))

#ifdef WIN32
#define MAX32	0x000000003fffffff
#define MAX64   0x3fffffffffffffff
#else
#define MAX32   0x000000003fffffffLL
#define MAX64   0x3fffffffffffffffLL
#endif

#define MIN32	-MAX32
#define MIN64	-MAX64

typedef u32_t wdigit;
#define BASE_TWODIGITS_TYPE long
typedef unsigned BASE_TWODIGITS_TYPE twodigits;
typedef BASE_TWODIGITS_TYPE stwodigits; /* signed variant of twodigits */

#define SHIFT	15
#define BASE	((digit)1 << SHIFT)
#define MASK	((int)(BASE - 1))

typedef digit*  digit_p;

#define long_size(obj)		(((long_p)((obj)->data.ptr))->size)
#define long_dsize(obj)		(((long_p)((obj)->data.ptr))->dsize)
#define long_digitp(obj)	(((long_p)((obj)->data.ptr))->digit)

#define int_sign(obj)													\
	( obj->data_type == DATA_TYPE_IMMDATA ?								\
		(obj->data.i64   == 0 ? 0 : (obj->data.i64   < 0 ? -1 : 1)) :	\
		(long_dsize(obj) == 0 ? 0 : (long_dsize(obj) < 0 ? -1 : 1)) )	\

#define SIGCHECK(x)			if (0) x
#define Py_CHARMASK(c)		((c) & 0xff)
#define Py_INCREF(x)		do {} while (0)
#define Py_XDECREF(x)		do {} while (0)

/* Forward */
static long_p long_normalize(long_p);
static long_p mul1(long_p, wdigit);
static long_p muladd1(long_p, wdigit, wdigit);
static long_p divrem1(long_p, digit, digit *);
obj_p  long_format(long_p aa, int base, int uppercase);

/* Long integer representation.
  The absolute value of a number is equal to
  	SUM(for i=0 through abs(dsize)-1) digit[i] * 2**(SHIFT*i)
  Negative numbers are represented with dsize < 0;
  zero is represented by dsize == 0.
  In a normalized number, digit[abs(dsize)-1] (the most significant
  digit) is never zero. Also, in all cases, for all valid i,
  	0 <= digit[i] <= MASK.
  The allocation function takes care of allocating extra memory
  so that digit[0] ... digit[abs(dsize)-1] are actually available.
*/

MODULE_DECLARE(Int);
MODULE_DECLARE(IntGen);

//********************************* new_int_obj *******************************
obj_p new_int_obj(isp ist, i64_t num){
	obj_p obj = NEW_OBJ(OBJ(INT_PROTO));
	SET_TYPE_IF_EXC(OBJ(INT_PROTO), obj, DATA_TYPE_IMMDATA) return NULL;
	obj->imm_data_len = 8;
	obj->data.i64 = num;
	obj->immutable = TRUE;
	return obj;
}

//********************************* new_int_long_obj **************************
obj_p new_int_long_obj(isp ist, long_p longp){
	int len;
	obj_p obj;
	long_normalize(longp);
	len            = sizeof(long_t) + abs(longp->dsize) * sizeof(digit);
	obj            = NEW_OBJ(Int_OBJ);

	SET_TYPE_IF_EXC(Int_OBJ, obj, DATA_TYPE_DATAPTR) return NULL;
	obj->data.ptr  = pr_malloc(len);
	memcpy(obj->data.ptr, longp, len);
	long_size(obj) = abs(longp->dsize);
	obj->immutable = TRUE;
	return obj;
}

int int_is_zero(obj_p obj) {
	if (obj->data_type  == DATA_TYPE_IMMDATA)
		return (obj->data.i64 == 0);
	else
		return (long_dsize(obj) == 0);
}

/* Normalize (remove leading zeros from) a int object.
  Doesn't attempt to free the storage--in most cases, due to the nature
  of the algorithms used, this could save at most be one word anyway. */
static long_p long_normalize(register long_p v) {
	int j = abs(v->dsize);
	register int i = j;

	while (i > 0 && v->digit[i-1] == 0)
		--i;
	if (i != j)
		v->dsize = (v->dsize < 0) ? -(i) : i;
	return v;
}

/* Allocate a new long int object with dsize digits.
  Return NULL and set exception if we run out of memory. */

long_p new_longp_non_init(u32_t size) {
	long_p longp = pr_malloc(sizeof(long_t) + size * sizeof(digit));
	longp->size = size;
	longp->dsize = size;
	return longp;
}

long_p copy_longp(long_p src, int size_padding) {
	long_p result;
	int i;
	pr_assert(src != NULL);
	i = abs(src->dsize) + size_padding;
	result = new_longp_non_init(i);
	if (result != NULL) {
		if (src->dsize >= 0) result->dsize =  i;
		else                 result->dsize = -i;
		while (--i >= 0)
			result->digit[i] = (i >= abs(src->dsize) ? 0 : src->digit[i]);
	}
	return (long_p)result;
}

/* Create a new long_p from a i64_t */
long_p new_longp(i64_t ival)
{
	long_p v;
	u64_t  t; /* unsigned so >> doesn't propagate sign bit */
	int ndigits = 0;
	int negative = 0;

	if (ival < 0) {
		ival = -ival;
		negative = 1;
	}
	/* Count the number of digits.
	  We used to pick 5 ("big enough for anything"), but that's a
	  waste of time and space given that 5*15 = 75 bits are rarely
	  needed. */
	t = (u64_t) ival;
	while (t) {
		++ndigits;
		t >>= SHIFT;
	}
	v = new_longp_non_init(ndigits);
	if (v != NULL) {
		digit *p = v->digit;
		v->dsize = negative ? -ndigits : ndigits;
		t = (unsigned long)ival;
		while (t) {
			*p++ = (digit)(t & MASK);
			t >>= SHIFT;
		}
	}
	return (long_p) v;
}

/* Create a new long int object from a C double */
long_p  double2longp(isp ist, double dval) {
	long_p v;
	double frac;
	int i, ndig, expo, neg;
	neg = 0;
	if (Py_IS_INFINITY(dval)) {
		raise_exception(ist, OBJ(OVERFLOW_EXC),
			"cannot convert float infinity to long");
		return NULL;
	}
	if (dval < 0.0) {
		neg = 1;
		dval = -dval;
	}
	frac = frexp(dval, &expo); /* dval = frac*2**expo; 0.0 <= frac < 1.0 */
	if (expo <= 0)
		return new_longp(0L);
	ndig = (expo-1) / SHIFT + 1; /* Number of 'digits' in result */
	v = new_longp_non_init(ndig);
	if (v == NULL)
		return NULL;
	frac = ldexp(frac, (expo-1) % SHIFT + 1);
	for (i = ndig; --i >= 0; ) {
		long bits = (long)frac;
		v->digit[i] = (digit) bits;
		frac = frac - (double)bits;
		frac = ldexp(frac, SHIFT);
	}
	if (neg)
		v->dsize = -(v->dsize);
	return (long_p)v;
}

/* Get a C long int from a long int object.
  Returns -1 and sets an error condition if overflow occurs. */

i64_t longp2int64(isp ist, long_p vv) {
	/* This version by Tim Peters */
	register long_p v;
	u64_t x, prev;
	int i, sign;

	v = (long_p) vv;
	i = v->dsize;
	sign = 1;
	x = 0;
	if (i < 0) {
		sign = -1;
		i = -(i);
	}
	while (--i >= 0) {
		prev = x;
		x = (x << SHIFT) + v->digit[i];
		if ((x >> SHIFT) != prev)
			goto overflow;
	}
	/* Haven't lost any bits, but if the sign bit is set we're in
	 * trouble *unless* this is the min negative number. So,
	 * trouble iff sign bit set && (positive || some bit set other
	 * than the sign bit).
	 */
	if ((i64_t) x < 0 && (sign > 0 || (x << 1) != 0))
		goto overflow;
	return (i64_t) x * sign;

 overflow:
	raise_exception(ist, OBJ(OVERFLOW_EXC),
			"long int too large to convert to int");
	return -1;
}

i32_t longp2int32(isp ist, long_p vv) {
	/* This version by Tim Peters */
	register long_p v;
	u64_t x, prev;
	int i, sign;

	v = (long_p) vv;
	i = v->dsize;
	sign = 1;
	x = 0;
	if (i < 0) {
		sign = -1;
		i = -(i);
	}
	while (--i >= 0) {
		prev = x;
		x = (x << SHIFT) + v->digit[i];
		if ((x >> SHIFT) != prev)
			goto overflow;
	}
	/* Haven't lost any bits, but if the sign bit is set we're in
	 * trouble *unless* this is the min negative number. So,
	 * trouble iff sign bit set && (positive || some bit set other
	 * than the sign bit).
	 */
	if ((i32_t) x < 0 && (sign > 0 || (x << 1) != 0))
		goto overflow;
	return (i32_t) x * sign;

 overflow:
	raise_exception(ist, OBJ(OVERFLOW_EXC),
			"long int too large to convert to int");
	return -1;
}

size_t _PyLong_NumBits(isp ist, long_p vv)
{
	long_p v = (long_p)vv;
	size_t result = 0;
	int ndigits;

	pr_assert(v != NULL);
	ndigits = abs(v->dsize);
	pr_assert(ndigits == 0 || v->digit[ndigits - 1] != 0);
	if (ndigits > 0) {
		digit msd = v->digit[ndigits - 1];

		result = (ndigits - 1) * SHIFT;
		if (result / SHIFT != (size_t)ndigits - 1)
			goto Overflow;
		do {
			++result;
			if (result == 0)
				goto Overflow;
			msd >>= 1;
		} while (msd);
	}
	return result;

Overflow:
	raise_exception(ist, OBJ(OVERFLOW_EXC), "long has too many bits "
			"to express in a platform size_t");
	return (size_t)-1;
}


double _PyLong_AsScaledDouble(long_p vv, int *exponent)
{
/* NBITS_WANTED should be > the number of bits in a double's precision,
  but small enough so that 2**NBITS_WANTED is within the normal double
  range. nbitsneeded is set to 1 less than that because the most-significant
  digit contains at least 1 significant bit, but we don't want to
  bother counting them (catering to the worst case cheaply).

  57 is one more than VAX-D double precision; I (Tim) don't know of a double
  format with more precision than that; it's 1 larger so that we add in at
  least one round bit to stand in for the ignored least-significant bits.
*/
#define NBITS_WANTED 57
	long_p v;
	double x;
	const double multiplier = (double)(1L << SHIFT);
	int i, sign;
	int nbitsneeded;

	v = (long_p)vv;
	i = v->dsize;
	sign = 1;
	if (i < 0) {
		sign = -1;
		i = -(i);
	}
	else if (i == 0) {
		*exponent = 0;
		return 0.0;
	}
	--i;
	x = (double)v->digit[i];
	nbitsneeded = NBITS_WANTED - 1;
	/* Invariant: i digits remain unaccounted for. */
	while (i > 0 && nbitsneeded > 0) {
		--i;
		x = x * multiplier + (double)v->digit[i];
		nbitsneeded -= SHIFT;
	}
	/* There are i digits we didn't shift in. Pretending they're all
	  zeroes, the true value is x * 2**(i*SHIFT). */
	*exponent = i;
	pr_assert(x > 0.0);
	return x * sign;
#undef NBITS_WANTED
}

/* Get a C double from a long int object. */

double PyLong_AsDouble(isp ist, long_p vv)
{
	int e;
	double x;

	x = _PyLong_AsScaledDouble(vv, &e);
	//if (x == -1.0 && PyErr_Occurred())
	//	return -1.0;
	if (e > INT_MAX / SHIFT)
		goto overflow;
	errno = 0;
	x = ldexp(x, e * SHIFT);
	if (Py_OVERFLOWED(x))
		goto overflow;
	return x;

overflow:
	raise_exception(ist, OBJ(OVERFLOW_EXC),
		"long int too large to convert to float");
	return -1.0;
}


#define CONVERT_BINOP(v, w, a, b) *a = copy_longp(long_normalize(v), 0); *b = copy_longp(long_normalize(w), 0);

/* x[0:m] and y[0:n] are digit vectors, LSD first, m >= n required. x[0:n]
 * is modified in place, by adding y to it. Carries are propagated as far as
 * x[m-1], and the remaining carry (0 or 1) is returned.
 */
static digit v_iadd(digit *x, int m, digit *y, int n) {
	int i;
	digit carry = 0;

	pr_assert(m >= n);
	for (i = 0; i < n; ++i) {
		carry += x[i] + y[i];
		x[i] = carry & MASK;
		carry >>= SHIFT;
		pr_assert((carry & 1) == carry);
	}
	for (; carry && i < m; ++i) {
		carry += x[i];
		x[i] = carry & MASK;
		carry >>= SHIFT;
		pr_assert((carry & 1) == carry);
	}
	return carry;
}

/* x[0:m] and y[0:n] are digit vectors, LSD first, m >= n required. x[0:n]
 * is modified in place, by subtracting y from it. Borrows are propagated as
 * far as x[m-1], and the remaining borrow (0 or 1) is returned.
 */
static digit v_isub(digit *x, int m, digit *y, int n)
{
	int i;
	digit borrow = 0;

	pr_assert(m >= n);
	for (i = 0; i < n; ++i) {
		borrow = x[i] - y[i] - borrow;
		x[i] = borrow & MASK;
		borrow >>= SHIFT;
		borrow &= 1;	/* keep only 1 sign bit */
	}
	for (; borrow && i < m; ++i) {
		borrow = x[i] - borrow;
		x[i] = borrow & MASK;
		borrow >>= SHIFT;
		borrow &= 1;
	}
	return borrow;
}

/* Multiply by a single digit, ignoring the sign. */

static long_p mul1(long_p a, wdigit n)
{
	return muladd1(a, n, (digit)0);
}

/* Multiply by a single digit and add a single digit, ignoring the sign. */

static long_p muladd1(long_p a, wdigit n, wdigit extra)
{
	int size_a = abs(a->dsize);
	long_p z = new_longp_non_init(size_a+1);
	twodigits carry = extra;
	int i;

	if (z == NULL)
		return NULL;
	for (i = 0; i < size_a; ++i) {
		carry += (twodigits)a->digit[i] * n;
		z->digit[i] = (digit) (carry & MASK);
		carry >>= SHIFT;
	}
	z->digit[i] = (digit) carry;
	return long_normalize(z);
}

/* Divide long pin, w/ dsize digits, by non-zero digit n, storing quotient
  in pout, and returning the remainder. pin and pout point at the LSD.
  It's OK for pin == pout on entry, which saves oodles of mallocs/frees in
  long_format, but that should be done with great care since longs are
  immutable. */

static digit inplace_divrem1(digit *pout, digit *pin, int dsize, digit n)
{
	twodigits rem = 0;

	pr_assert(n > 0 && n <= MASK);
	pin += dsize;
	pout += dsize;
	while (--dsize >= 0) {
		digit hi;
		rem = (rem << SHIFT) + *--pin;
		*--pout = hi = (digit)(rem / n);
		rem -= hi * n;
	}
	return (digit)rem;
}

/* Divide a long integer by a digit, returning both the quotient
  (as function result) and the remainder (through *prem).
  The sign of a is ignored; n should not be zero. */

static long_p divrem1(long_p a, digit n, digit *prem)
{
	const int dsize = abs(a->dsize);
	long_p z;

	pr_assert(n > 0 && n <= MASK);
	z = new_longp_non_init(dsize);
	if (z == NULL)
		return NULL;
	*prem = inplace_divrem1(z->digit, a->digit, dsize, n);
	return long_normalize(z);
}

/* Convert a long int object to a string, using a given conversion base.
  Return a string object.
  If base is 8 or 16, add the proper prefix '0' or '0x'. */
obj_p long_format(long_p aa, int base, int uppercase) {
	register long_p a = (long_p) aa;
	obj_p str;
	int i;
	const int size_a = abs(a->dsize);
	char base_a, *p, *strp;
	int bits;
	char sign = '\0';

	if (uppercase) 
		base_a = 'A';
	else 
		base_a = 'a';

	pr_assert(base >= 2 && base <= 36);

	/* Compute a rough upper bound for the length of the string */
	i = base;
	bits = 0;
	while (i > 1) {
		++bits;
		i >>= 1;
	}
	i = 5 + (size_a*SHIFT + bits-1) / bits;
	strp = pr_malloc(i+1);
	strp[i] = 0;
	p = strp + i;
	if (a->dsize < 0)
		sign = '-';

	if (a->dsize == 0) {
		*--p = '0';
	}
	else if ((base & (base - 1)) == 0) {
		/* JRH: special case for power-of-2 bases */
		twodigits accum = 0;
		int accumbits = 0;	/* # of bits in accum */
		int basebits = 1;	/* # of bits in base-1 */
		i = base;
		while ((i >>= 1) > 1)
			++basebits;

		for (i = 0; i < size_a; ++i) {
			accum |= (twodigits)a->digit[i] << accumbits;
			accumbits += SHIFT;
			pr_assert(accumbits >= basebits);
			do {
				char cdigit = (char)(accum & (base - 1));
				cdigit += (cdigit < 10) ? '0' : base_a-10;
				pr_assert(p > strp);
				*--p = cdigit;
				accumbits -= basebits;
				accum >>= basebits;
			} while (i < size_a-1 ? accumbits >= basebits :
					 	accum > 0);
		}
	}
	else {
		/* Not 0, and base not a power of 2. Divide repeatedly by
		  base, but for speed use the highest power of base that
		  fits in a digit. */
		int dsize = size_a;
		digit *pin = a->digit;
		long_p scratch;
		/* powbasw <- largest power of base that fits in a digit. */
		digit powbase = base; /* powbase == base ** power */
		int power = 1;
		for (;;) {
			unsigned long newpow = powbase * (unsigned long)base;
			if (newpow >> SHIFT) /* doesn't fit in a digit */
				break;
			powbase = (digit)newpow;
			++power;
		}

		/* Get a scratch area for repeated division. */
		scratch = new_longp_non_init(dsize);
		if (scratch == NULL) {
			pr_free(strp);
			return NULL;
		}

		/* Repeatedly divide by powbase. */
		do {
			int ntostore = power;
			digit rem = inplace_divrem1(scratch->digit,
						   pin, dsize, powbase);
			pin = scratch->digit; /* no need to use a again */
			if (pin[dsize - 1] == 0)
				--dsize;
			SIGCHECK({
				pr_free(scratch);
				pr_free(strp);
				return NULL;
			})

			/* Break rem into digits. */
			pr_assert(ntostore > 0);
			do {
				digit nextrem = (digit)(rem / base);
				char c = (char)(rem - nextrem * base);
				pr_assert(p > strp);
				c += (c < 10) ? '0' : base_a-10;
				*--p = c;
				rem = nextrem;
				--ntostore;
				/* Termination is a bit delicate: must not
				  store leading zeroes, so must get out if
				  remaining quotient and rem are both 0. */
			} while (ntostore && (dsize || rem));
		} while (dsize != 0);
		pr_free(scratch);
	}

/*	if (base == 8) {
		if (size_a != 0)
			*--p = '0';
	}
	else if (base == 16) {
		*--p = 'x';
		*--p = '0';
	}
	else if (base != 10) {
		*--p = '#';
		*--p = '0' + base%10;
		if (base > 10)
			*--p = '0' + base/10;
	} */
	if (sign)
		*--p = sign;
	if (p != strp) {
		char *q = strp;
		pr_assert(p > q);
		do {
		} while ((*q++ = *p++) != '\0');
	}
	str = NEW_STRING(strp);
	pr_free(strp);
	return str;
}

/* *str points to the first digit in a string of base base digits. base
 * is a power of 2 (2, 4, 8, 16, or 32). *str is set to point to the first
 * non-digit (which may be *str!). A normalized long is returned.
 * The point to this routine is that it takes time linear in the number of
 * string characters.
 */
static long_p long_from_binary_base(isp ist, char **str, int base) {
	char *p = *str;
	char *start = p;
	int bits_per_char;
	int n;
	long_p z;
	twodigits accum;
	int bits_in_accum;
	digit *pdigit;

	pr_assert(base >= 2 && base <= 32 && (base & (base - 1)) == 0);
	n = base;
	for (bits_per_char = -1; n; ++bits_per_char)
		n >>= 1;
	/* n <- total # of bits needed, while setting p to end-of-string */
	n = 0;
	for (;;) {
		int k = -1;
		char ch = *p;

		if (ch <= '9')
			k = ch - '0';
		else if (ch >= 'a')
			k = ch - 'a' + 10;
		else if (ch >= 'A')
			k = ch - 'A' + 10;
		if (k < 0 || k >= base)
			break;
		++p;
	}
	*str = p;
	n = (int) (p - start) * bits_per_char;
	if (n / bits_per_char != p - start) {
		raise_exception(ist, OBJ(VALUE_EXC),
				"long string too large to convert");
		return NULL;
	}
	/* n <- # of digits needed, = ceiling(n/SHIFT). */
	n = (n + SHIFT - 1) / SHIFT;
	z = new_longp_non_init(n);
	if (z == NULL)
		return NULL;
	/* Read string from right, and fill in long from left; i.e.,
	 * from least to most significant in both.
	 */
	accum = 0;
	bits_in_accum = 0;
	pdigit = z->digit;
	while (--p >= start) {
		int k;
		char ch = *p;

		if (ch <= '9')
			k = ch - '0';
		else if (ch >= 'a')
			k = ch - 'a' + 10;
		else {
			pr_assert(ch >= 'A');
			k = ch - 'A' + 10;
		}
		pr_assert(k >= 0 && k < base);
		accum |= (twodigits)(k << bits_in_accum);
		bits_in_accum += bits_per_char;
		if (bits_in_accum >= SHIFT) {
			*pdigit++ = (digit)(accum & MASK);
			pr_assert(pdigit - z->digit <= n);
			accum >>= SHIFT;
			bits_in_accum -= SHIFT;
			pr_assert(bits_in_accum < SHIFT);
		}
	}
	if (bits_in_accum) {
		pr_assert(bits_in_accum <= SHIFT);
		*pdigit++ = (digit)accum;
		pr_assert(pdigit - z->digit <= n);
	}
	while (pdigit - z->digit < n)
		*pdigit++ = 0;
	return long_normalize(z);
}

long_p cstr2longp(isp ist, char *str, char **pend, int base)
{
	int sign = 1;
	char *start, *orig_str = str;
	long_p z;

	if ((base != 0 && base < 2) || base > 36) {
		raise_exception(ist, OBJ(VALUE_EXC),
				"long() arg 2 must be >= 2 and <= 36");
		return NULL;
	}
	while (*str != '\0' && isspace(Py_CHARMASK(*str)))
		str++;
	if (*str == '+')
		++str;
	else if (*str == '-') {
		++str;
		sign = -1;
	}
	while (*str != '\0' && isspace(Py_CHARMASK(*str)))
		str++;
	if (base == 0) {
		if (str[0] != '0')
			base = 10;
		else if (str[1] == 'x' || str[1] == 'X')
			base = 16;
		else
			base = 8;
	}
	if (base == 16 && str[0] == '0' && (str[1] == 'x' || str[1] == 'X'))
		str += 2;
	start = str;
	if ((base & (base - 1)) == 0)
		z = long_from_binary_base(ist, &str, base);
	else {
		z = new_longp_non_init(0);
		for ( ; z != NULL; ++str) {
			int k = -1;
			long_p temp;

			if (*str <= '9')
				k = *str - '0';
			else if (*str >= 'a')
				k = *str - 'a' + 10;
			else if (*str >= 'A')
				k = *str - 'A' + 10;
			if (k < 0 || k >= base)
				break;
			temp = muladd1(z, (digit)base, (digit)k);
			pr_free(z);
			z = temp;
		}
	}
	if (z == NULL)
		return NULL;
	if (str == start)
		goto onError;
	if (sign < 0 && z != NULL && z->dsize != 0)
		z->dsize = -(z->dsize);
	if (*str == 'L' || *str == 'l')
		str++;
	while (*str && isspace(Py_CHARMASK(*str)))
		str++;
	if (*str != '\0')
		goto onError;
	if (pend)
		*pend = str;
	return (long_p) z;

 onError:
	raise_exception(ist, OBJ(VALUE_EXC),
		   "invalid literal for long(): %.200s", orig_str);
	Py_XDECREF(z);
	return NULL;
}

#ifdef Py_USING_UNICODE
long_p PyLong_FromUnicode(isp ist, Py_UNICODE *u, int length, int base)
{
	long_p result;
	char *buffer = PyMem_MALLOC(length+1);

	if (buffer == NULL)
		return NULL;

	if (PyUnicode_EncodeDecimal(u, length, buffer, NULL)) {
		PyMem_FREE(buffer);
		return NULL;
	}
	result = cstr2longp(ist, buffer, NULL, base);
	PyMem_FREE(buffer);
	return result;
}
#endif

/* forward */
static long_p x_divrem(long_p, long_p, long_p *);

/* Long division with remainder, top-level routine */

static int long_divrem(isp ist, long_p a, long_p b, long_p *pdiv, long_p *prem) {
	int size_a = abs(a->dsize), size_b = abs(b->dsize);
	long_p z;

	if (size_b == 0) {
		raise_exception(ist, OBJ(DIVIDEZERO_EXC),
				"long division or modulo by zero");
		return -1;
	}
	if (size_a < size_b ||
	  (size_a == size_b &&
	   a->digit[size_a-1] < b->digit[size_b-1])) {
		/* |a| < |b|. */
		*pdiv = new_longp_non_init(0);
		Py_INCREF(a);
		*prem = copy_longp(a, 0);
		return 0;
	}
	if (size_b == 1) {
		digit rem = 0;
		z = divrem1(a, b->digit[0], &rem);
		if (z == NULL)
			return -1;
		*prem = (long_p) new_longp((long)rem);
	}
	else {
		z = x_divrem(a, b, prem);
		if (z == NULL)
			return -1;
	}
	/* Set the signs.
	  The quotient z has the sign of a*b;
	  the remainder r has the sign of a,
	  so a = b*z + r. */
	if ((a->dsize < 0) != (b->dsize < 0))
		z->dsize = -(z->dsize);
	if (a->dsize < 0 && (*prem)->dsize != 0)
		(*prem)->dsize = -((*prem)->dsize);
	*pdiv = z;
	return 0;
}

/* Unsigned long division with remainder -- the algorithm */

static long_p x_divrem(long_p v1, long_p w1, long_p *prem)
{
	int size_v = abs(v1->dsize), size_w = abs(w1->dsize);
	digit  d = (digit) ((twodigits)BASE / (w1->digit[size_w-1] + 1));
	long_p v = mul1(v1, d);
	long_p w = mul1(w1, d);
	long_p a;
	int j, k;

	if (v == NULL || w == NULL) {
		Py_XDECREF(v);
		Py_XDECREF(w);
		return NULL;
	}

	pr_assert(size_v >= size_w && size_w > 1); /* Assert checks by div() */
	pr_assert(size_w == abs(w->dsize)); /* That's how d was calculated */

	size_v = abs(v->dsize);
	a = new_longp_non_init(size_v - size_w + 1);

	for (j = size_v, k = a->dsize-1; a != NULL && k >= 0; --j, --k) {
		digit vj = (j >= size_v) ? 0 : v->digit[j];
		twodigits q;
		stwodigits carry = 0;
		int i;

		SIGCHECK({
			pr_free(a);
			a = NULL;
			break;
		})
		if (vj == w->digit[size_w-1])
			q = MASK;
		else
			q = (((twodigits)vj << SHIFT) + v->digit[j-1]) /
				w->digit[size_w-1];

		while (w->digit[size_w-2]*q >
				((
					((twodigits)vj << SHIFT)
					+ v->digit[j-1]
					- q*w->digit[size_w-1]
								) << SHIFT)
				+ v->digit[j-2])
			--q;

		for (i = 0; i < size_w && i+k < size_v; ++i) {
			twodigits z = w->digit[i] * q;
			digit zz = (digit) (z >> SHIFT);
			carry += v->digit[i+k] - z
				+ ((twodigits)zz << SHIFT);
			v->digit[i+k] = (digit)(carry & MASK);
			carry = PR_ARITHMETIC_RIGHT_SHIFT(BASE_TWODIGITS_TYPE,
							 carry, SHIFT);
			carry -= zz;
		}

		if (i+k < size_v) {
			carry += v->digit[i+k];
			v->digit[i+k] = 0;
		}

		if (carry == 0)
			a->digit[k] = (digit) q;
		else {
			pr_assert(carry == -1);
			a->digit[k] = (digit) q-1;
			carry = 0;
			for (i = 0; i < size_w && i+k < size_v; ++i) {
				carry += v->digit[i+k] + w->digit[i];
				v->digit[i+k] = (digit)(carry & MASK);
				carry = PR_ARITHMETIC_RIGHT_SHIFT(
						BASE_TWODIGITS_TYPE,
						carry, SHIFT);
			}
		}
	} /* for j, k */

	if (a == NULL)
		*prem = NULL;
	else {
		a = long_normalize(a);
		*prem = divrem1(v, d, &d);
		/* d receives the (unused) remainder */
		if (*prem == NULL) {
			pr_free(a);
			a = NULL;
		}
	}
	pr_free(v);
	pr_free(w);
	return a;
}

/* Methods */


static int long_compare(long_p a, long_p b)
{
	int sign;

	if (a->dsize != b->dsize) {
		if (abs(a->dsize) == 0 && abs(b->dsize) == 0)
			sign = 0;
		else
			sign = a->dsize - b->dsize;
	}
	else {
		int i = abs(a->dsize);
		while (--i >= 0 && a->digit[i] == b->digit[i])
			;
		if (i < 0)
			sign = 0;
		else {
			sign = (int)a->digit[i] - (int)b->digit[i];
			if (a->dsize < 0)
				sign = -sign;
		}
	}
	return sign < 0 ? -1 : sign > 0 ? 1 : 0;
}

static long_p x_add(long_p a, long_p b)
{
	int size_a = abs(a->dsize), size_b = abs(b->dsize);
	long_p z;
	int i;
	digit carry = 0;

	/* Ensure a is the larger of the two: */
	if (size_a < size_b) {
		{ long_p temp = a; a = b; b = temp; }
		{ int size_temp = size_a;
		 size_a = size_b;
		 size_b = size_temp; }
	}
	z = new_longp_non_init(size_a+1);
	if (z == NULL)
		return NULL;
	for (i = 0; i < size_b; ++i) {
		carry += a->digit[i] + b->digit[i];
		z->digit[i] = carry & MASK;
		carry >>= SHIFT;
	}
	for (; i < size_a; ++i) {
		carry += a->digit[i];
		z->digit[i] = carry & MASK;
		carry >>= SHIFT;
	}
	z->digit[i] = carry;
	return long_normalize(z);
}

/* Subtract the absolute values of two integers. */

static long_p x_sub(long_p a, long_p b)
{
	int size_a = abs(a->dsize), size_b = abs(b->dsize);
	long_p z;
	int i;
	int sign = 1;
	digit borrow = 0;

	/* Ensure a is the larger of the two: */
	if (size_a < size_b) {
		sign = -1;
		{ long_p temp = a; a = b; b = temp; }
		{ int size_temp = size_a;
		 size_a = size_b;
		 size_b = size_temp; }
	}
	else if (size_a == size_b) {
		/* Find highest digit where a and b differ: */
		i = size_a;
		while (--i >= 0 && a->digit[i] == b->digit[i])
			;
		if (i < 0)
			return new_longp_non_init(0);
		if (a->digit[i] < b->digit[i]) {
			sign = -1;
			{ long_p temp = a; a = b; b = temp; }
		}
		size_a = size_b = i+1;
	}
	z = new_longp_non_init(size_a);
	if (z == NULL)
		return NULL;
	for (i = 0; i < size_b; ++i) {
		/* The following assumes unsigned arithmetic
		  works module 2**N for some N>SHIFT. */
		borrow = a->digit[i] - b->digit[i] - borrow;
		z->digit[i] = borrow & MASK;
		borrow >>= SHIFT;
		borrow &= 1; /* Keep only one sign bit */
	}
	for (; i < size_a; ++i) {
		borrow = a->digit[i] - borrow;
		z->digit[i] = borrow & MASK;
		borrow >>= SHIFT;
		borrow &= 1; /* Keep only one sign bit */
	}
	pr_assert(borrow == 0);
	if (sign < 0)
		z->dsize = -(z->dsize);
	return long_normalize(z);
}

static long_p long_add(long_p v, long_p w)
{
	long_p a, b, z;

	CONVERT_BINOP((long_p)v, (long_p)w, &a, &b);

	if (a->dsize < 0) {
		if (b->dsize < 0) {
			z = x_add(a, b);
			if (z != NULL && z->dsize != 0)
				z->dsize = -(z->dsize);
		}
		else
			z = x_sub(b, a);
	}
	else {
		if (b->dsize < 0)
			z = x_sub(a, b);
		else
			z = x_add(a, b);
	}
	pr_free(a);
	pr_free(b);
	return (long_p)z;
}

static long_p long_sub(long_p v, long_p w)
{
	long_p a, b, z;

	CONVERT_BINOP((long_p)v, (long_p)w, &a, &b);

	if (a->dsize < 0) {
		if (b->dsize < 0)
			z = x_sub(a, b);
		else
			z = x_add(a, b);
		if (z != NULL && z->dsize != 0)
			z->dsize = -(z->dsize);
	}
	else {
		if (b->dsize < 0)
			z = x_add(a, b);
		else
			z = x_sub(a, b);
	}
	pr_free(a);
	pr_free(b);
	return (long_p)z;
}

/* Grade school multiplication, ignoring the signs.
 * Returns the absolute value of the product, or NULL if error.
 */
static long_p x_mul(long_p a, long_p b)
{
	long_p z;
	int size_a = abs(a->dsize);
	int size_b = abs(b->dsize);
	int i;

   	z = new_longp_non_init(size_a + size_b);
	if (z == NULL)
		return NULL;

	memset(z->digit, 0, z->size * sizeof(digit));
	for (i = 0; i < size_a; ++i) {
		twodigits carry = 0;
		twodigits f = a->digit[i];
		int j;
		digit *pz = z->digit + i;

		for (j = 0; j < size_b; ++j) {
			carry += *pz + b->digit[j] * f;
			*pz++ = (digit) (carry & MASK);
			carry >>= SHIFT;
		}
		for (; carry != 0; ++j) {
			pr_assert(i+j < z->dsize);
			carry += *pz;
			*pz++ = (digit) (carry & MASK);
			carry >>= SHIFT;
		}
	}
	return long_normalize(z);
}

/* A helper for Karatsuba multiplication (k_mul).
  Takes a long "n" and an integer "dsize" representing the place to
  split, and sets low and high such that abs(n) == (high << dsize) + low,
  viewing the shift as being by digits. The sign bit is ignored, and
  the return values are >= 0.
  Returns 0 on success, -1 on failure.
*/
static int kmul_split(long_p n, int dsize, long_p *high, long_p *low)
{
	long_p hi, lo;
	int size_lo, size_hi;
	const int size_n = abs(n->dsize);

	size_lo = min(size_n, dsize);
	size_hi = size_n - size_lo;

	if ((hi = new_longp_non_init(size_hi)) == NULL)
		return -1;
	if ((lo = new_longp_non_init(size_lo)) == NULL) {
		pr_free(hi);
		return -1;
	}

	memcpy(lo->digit, n->digit, size_lo * sizeof(digit));
	memcpy(hi->digit, n->digit + size_lo, size_hi * sizeof(digit));

	*high = long_normalize(hi);
	*low = long_normalize(lo);
	return 0;
}

static long_p k_lopsided_mul(long_p a, long_p b);

/* Karatsuba multiplication. Ignores the input signs, and returns the
 * absolute value of the product (or NULL if error).
 * See Knuth Vol. 2 Chapter 4.3.3 (Pp. 294-295).
 */
static long_p k_mul(long_p a, long_p b)
{
	int asize = abs(a->dsize);
	int bsize = abs(b->dsize);
	long_p ah = NULL;
	long_p al = NULL;
	long_p bh = NULL;
	long_p bl = NULL;
	long_p ret = NULL;
	long_p t1, t2, t3;
	int shift;	/* the number of digits we split off */
	int i;

	/* (ah*X+al)(bh*X+bl) = ah*bh*X*X + (ah*bl + al*bh)*X + al*bl
	 * Let k = (ah+al)*(bh+bl) = ah*bl + al*bh + ah*bh + al*bl
	 * Then the original product is
	 *   ah*bh*X*X + (k - ah*bh - al*bl)*X + al*bl
	 * By picking X to be a power of 2, "*X" is just shifting, and it's
	 * been reduced to 3 multiplies on numbers half the dsize.
	 */

	/* We want to split based on the larger number; fiddle so that b
	 * is largest.
	 */
	if (asize > bsize) {
		t1 = a;
		a = b;
		b = t1;

		i = asize;
		asize = bsize;
		bsize = i;
	}

	/* Use gradeschool math when either number is too small. */
	if (asize <= KARATSUBA_CUTOFF) {
		if (asize == 0)
			return new_longp_non_init(0);
		else
			return x_mul(a, b);
	}

	/* If a is small compared to b, splitting on b gives a degenerate
	 * case with ah==0, and Karatsuba may be (even much) less efficient
	 * than "grade school" then. However, we can still win, by viewing
	 * b as a string of "big digits", each of width a->dsize. That
	 * leads to a sequence of balanced calls to k_mul.
	 */
	if (2 * asize <= bsize)
		return k_lopsided_mul(a, b);

	/* Split a & b into hi & lo pieces. */
	shift = bsize >> 1;
	if (kmul_split(a, shift, &ah, &al) < 0) goto fail;
	pr_assert(ah->dsize > 0);	/* the split isn't degenerate */

	if (kmul_split(b, shift, &bh, &bl) < 0) goto fail;

	/* The plan:
	 * 1. Allocate result space (asize + bsize digits: that's always
	 *  enough).
	 * 2. Compute ah*bh, and copy into result at 2*shift.
	 * 3. Compute al*bl, and copy into result at 0. Note that this
	 *  can't overlap with #2.
	 * 4. Subtract al*bl from the result, starting at shift. This may
	 *  underflow (borrow out of the high digit), but we don't care:
	 *  we're effectively doing unsigned arithmetic mod
	 *  BASE**(sizea + sizeb), and so long as the *final* result fits,
	 *  borrows and carries out of the high digit can be ignored.
	 * 5. Subtract ah*bh from the result, starting at shift.
	 * 6. Compute (ah+al)*(bh+bl), and add it into the result starting
	 *  at shift.
	 */

	/* 1. Allocate result space. */
	ret = new_longp_non_init(asize + bsize);
	if (ret == NULL) goto fail;
#ifdef Py_DEBUG
	/* Fill with trash, to catch reference to uninitialized digits. */
	memset(ret->digit, 0xDF, ret->dsize * sizeof(digit));
#endif

	/* 2. t1 <- ah*bh, and copy into high digits of result. */
	if ((t1 = k_mul(ah, bh)) == NULL) goto fail;
	pr_assert(t1->dsize >= 0);
	pr_assert(2*shift + t1->dsize <= ret->dsize);
	memcpy(ret->digit + 2*shift, t1->digit,
	    t1->dsize * sizeof(digit));

	/* Zero-out the digits higher than the ah*bh copy. */
	i = ret->dsize - 2*shift - t1->dsize;
	if (i)
		memset(ret->digit + 2*shift + t1->dsize, 0,
		    i * sizeof(digit));

	/* 3. t2 <- al*bl, and copy into the low digits. */
	if ((t2 = k_mul(al, bl)) == NULL) {
		pr_free(t1);
		goto fail;
	}
	pr_assert(t2->dsize >= 0);
	pr_assert(t2->dsize <= 2*shift); /* no overlap with high digits */
	memcpy(ret->digit, t2->digit, t2->dsize * sizeof(digit));

	/* Zero out remaining digits. */
	i = 2*shift - t2->dsize;	/* number of uninitialized digits */
	if (i)
		memset(ret->digit + t2->dsize, 0, i * sizeof(digit));

	/* 4 & 5. Subtract ah*bh (t1) and al*bl (t2). We do al*bl first
	 * because it's fresher in cache.
	 */
	i = ret->dsize - shift; /* # digits after shift */
	(void)v_isub(ret->digit + shift, i, t2->digit, t2->dsize);
	pr_free(t2);

	(void)v_isub(ret->digit + shift, i, t1->digit, t1->dsize);
	pr_free(t1);

	/* 6. t3 <- (ah+al)(bh+bl), and add into result. */
	if ((t1 = x_add(ah, al)) == NULL) goto fail;
	pr_free(ah);
	pr_free(al);
	ah = al = NULL;

	if ((t2 = x_add(bh, bl)) == NULL) {
		pr_free(t1);
		goto fail;
	}
	pr_free(bh);
	pr_free(bl);
	bh = bl = NULL;

	t3 = k_mul(t1, t2);
	pr_free(t1);
	pr_free(t2);
	if (t3 == NULL) goto fail;
	pr_assert(t3->dsize >= 0);

	/* Add t3. It's not obvious why we can't run out of room here.
	 * See the (*) comment after this function.
	 */
	(void)v_iadd(ret->digit + shift, i, t3->digit, t3->dsize);
	pr_free(t3);

	return long_normalize(ret);

 fail:
 	Py_XDECREF(ret);
	Py_XDECREF(ah);
	Py_XDECREF(al);
	Py_XDECREF(bh);
	Py_XDECREF(bl);
	return NULL;
}

/* (*) Why adding t3 can't "run out of room" above.

Let f(x) mean the floor of x and c(x) mean the ceiling of x. Some facts
to start with:

1. For any integer i, i = c(i/2) + f(i/2). In particular,
  bsize = c(bsize/2) + f(bsize/2).
2. shift = f(bsize/2)
3. asize <= bsize
4. Since we call k_lopsided_mul if asize*2 <= bsize, asize*2 > bsize in this
  routine, so asize > bsize/2 >= f(bsize/2) in this routine.

We allocated asize + bsize result digits, and add t3 into them at an offset
of shift. This leaves asize+bsize-shift allocated digit positions for t3
to fit into, = (by #1 and #2) asize + f(bsize/2) + c(bsize/2) - f(bsize/2) =
asize + c(bsize/2) available digit positions.

bh has c(bsize/2) digits, and bl at most f(dsize/2) digits. So bh+hl has
at most c(bsize/2) digits + 1 bit.

If asize == bsize, ah has c(bsize/2) digits, else ah has at most f(bsize/2)
digits, and al has at most f(bsize/2) digits in any case. So ah+al has at
most (asize == bsize ? c(bsize/2) : f(bsize/2)) digits + 1 bit.

The product (ah+al)*(bh+bl) therefore has at most

  c(bsize/2) + (asize == bsize ? c(bsize/2) : f(bsize/2)) digits + 2 bits

and we have asize + c(bsize/2) available digit positions. We need to show
this is always enough. An instance of c(bsize/2) cancels out in both, so
the question reduces to whether asize digits is enough to hold
(asize == bsize ? c(bsize/2) : f(bsize/2)) digits + 2 bits. If asize < bsize,
then we're asking whether asize digits >= f(bsize/2) digits + 2 bits. By #4,
asize is at least f(bsize/2)+1 digits, so this in turn reduces to whether 1
digit is enough to hold 2 bits. This is so since SHIFT=15 >= 2. If
asize == bsize, then we're asking whether bsize digits is enough to hold
c(bsize/2) digits + 2 bits, or equivalently (by #1) whether f(bsize/2) digits
is enough to hold 2 bits. This is so if bsize >= 2, which holds because
bsize >= KARATSUBA_CUTOFF >= 2.

Note that since there's always enough room for (ah+al)*(bh+bl), and that's
clearly >= each of ah*bh and al*bl, there's always enough room to subtract
ah*bh and al*bl too.
*/

/* b has at least twice the digits of a, and a is big enough that Karatsuba
 * would pay off *if* the inputs had balanced sizes. View b as a sequence
 * of slices, each with a->dsize digits, and multiply the slices by a,
 * one at a time. This gives k_mul balanced inputs to work with, and is
 * also cache-friendly (we compute one double-width slice of the result
 * at a time, then move on, never bactracking except for the helpful
 * single-width slice overlap between successive partial sums).
 */
static long_p k_lopsided_mul(long_p a, long_p b)
{
	const int asize = abs(a->dsize);
	int bsize = abs(b->dsize);
	int nbdone;	/* # of b digits already multiplied */
	long_p ret;
	long_p bslice = NULL;

	pr_assert(asize > KARATSUBA_CUTOFF);
	pr_assert(2 * asize <= bsize);

	/* Allocate result space, and zero it out. */
	ret = new_longp_non_init(asize + bsize);
	if (ret == NULL)
		return NULL;
	memset(ret->digit, 0, ret->dsize * sizeof(digit));

	/* Successive slices of b are copied into bslice. */
	bslice = new_longp_non_init(asize);
	if (bslice == NULL)
		goto fail;

	nbdone = 0;
	while (bsize > 0) {
		long_p product;
		const int nbtouse = min(bsize, asize);

		/* Multiply the next slice of b by a. */
		memcpy(bslice->digit, b->digit + nbdone,
		    nbtouse * sizeof(digit));
		bslice->dsize = nbtouse;
		product = k_mul(a, bslice);
		if (product == NULL)
			goto fail;

		/* Add into result. */
		(void)v_iadd(ret->digit + nbdone, ret->dsize - nbdone,
			   product->digit, product->dsize);
		pr_free(product);

		bsize -= nbtouse;
		nbdone += nbtouse;
	}

	pr_free(bslice);
	return long_normalize(ret);

 fail:
	pr_free(ret);
	Py_XDECREF(bslice);
	return NULL;
}

static long_p long_mul(long_p v, long_p w) {
	long_p a = copy_longp(v, 1), b = copy_longp(w, 1), z;
	
	z = k_mul(a, b);
	/* Negate if exactly one of the inputs is negative. */
	if (((a->dsize ^ b->dsize) < 0) && z)
		z->dsize = -(z->dsize);
	pr_free(a);
	pr_free(b);
	return (long_p)z;
}

/* The / and % operators are now defined in terms of divmod().
  The expression a mod b has the value a - b*floor(a/b).
  The long_divrem function gives the remainder after division of
  |a| by |b|, with the sign of a. This is also expressed
  as a - b*trunc(a/b), if trunc truncates towards zero.
  Some examples:
  	 a	 b	a rem b		a mod b
  	 13	 10	 3		 3
  	-13	 10	-3		 7
  	 13	-10	 3		-7
  	-13	-10	-3		-3
  So, to get from rem to mod, we have to add b if a and b
  have different signs. We then subtract one from the 'div'
  part of the outcome to keep the invariant intact. */

static int l_divmod(isp ist, long_p v, long_p w, long_p *pdiv, long_p *pmod)
{
	long_p div, mod;

	if (long_divrem(ist, v, w, &div, &mod) < 0)
		return -1;
	if ((mod->dsize < 0 && w->dsize > 0) ||
	  (mod->dsize > 0 && w->dsize < 0)) {
		long_p temp;
		long_p one;
		temp = (long_p) long_add(mod, w);
		pr_free(mod);
		mod = temp;
		if (mod == NULL) {
			pr_free(div);
			return -1;
		}
		one = (long_p) new_longp(1);
		if (one == NULL ||
		  (temp = (long_p) long_sub(div, one)) == NULL) {
			pr_free(mod);
			pr_free(div);
			Py_XDECREF(one);
			return -1;
		}
		pr_free(one);
		pr_free(div);
		div = temp;
	}
	*pdiv = div;
	*pmod = mod;
	return 0;
}

static long_p long_div(long_p v, long_p w)
{
	long_p a, b, div, mod;

	CONVERT_BINOP(v, w, &a, &b);

	if (l_divmod(ist, a, b, &div, &mod) < 0) {
		pr_free(a);
		pr_free(b);
		return NULL;
	}
	pr_free(a);
	pr_free(b);
	pr_free(mod);
	return (long_p) div;
}

static long_p long_mod(isp ist, long_p v, long_p w)
{
	long_p a, b, div, mod;

	CONVERT_BINOP(v, w, &a, &b);

	if (l_divmod(ist, a, b, &div, &mod) < 0) {
		pr_free(a);
		pr_free(b);
		return NULL;
	}
	pr_free(a);
	pr_free(b);
	pr_free(div);
	return (long_p) mod;
}

// w must be positive
// handle negative using floats without calling this
static long_p long_pow(isp ist, long_p v, long_p w, long_p x)
{
	long_p a, b;
	long_p c;
	long_p z, div, mod;
	int size_b, i;

	CONVERT_BINOP(v, w, &a, &b);
	c = x;
	if (c && ((long_p)c)->dsize == 0) {
		raise_exception(ist, OBJ(VALUE_EXC),
				"pow() 3rd argument cannot be 0");
		z = NULL;
		goto error;
	}
	size_b = b->dsize;
	z = (long_p)new_longp(1);
	for (i = 0; i < size_b; ++i) {
		digit bi = b->digit[i];
		int j;

		for (j = 0; j < SHIFT; ++j) {
			long_p temp;

			if (bi & 1) {
				temp = (long_p)long_mul(z, a);
			 	if (c && temp!=NULL) {
			 		if (l_divmod(ist, temp,(long_p)c,
							&div,&mod) < 0) {
						pr_free(temp);
						pr_free(z);
						z = NULL;
						goto error;
					}
				 	Py_XDECREF(div);
				 	pr_free(temp);
				 	temp = mod;
				}
				pr_free(z);
			 	z = temp;
				if (z == NULL)
					break;
			}
			bi >>= 1;
			if (bi == 0 && i+1 == size_b)
				break;
			temp = (long_p)long_mul(a, a);
		 	if (c && temp!=NULL) {
			 	if (l_divmod(ist, temp, (long_p)c, &div,
							&mod) < 0) {
					pr_free(temp);
					pr_free(z);
					z = NULL;
					goto error;
				}
			 	Py_XDECREF(div);
			 	pr_free(temp);
			 	temp = mod;
			}
//			pr_free(a);
			a = temp;
			if (a == NULL) {
//				pr_free(z);
				z = NULL;
				break;
			}
		}
		if (a == NULL || z == NULL)
			break;
	}
	if (c && z!=NULL) {
		if (l_divmod(ist, z, (long_p)c, &div, &mod) < 0) {
//			pr_free(z);
			z = NULL;
		}
		else {
			Py_XDECREF(div);
			pr_free(z);
			z = mod;
		}
	}
 error:
	Py_XDECREF(a);
	pr_free(b);
	pr_free(c);
	return (long_p) z;
}

static long_p long_invert(isp ist, long_p v)
{
	/* Implement ~x as -(x+1) */
	long_p x;
	long_p w;
	w = (long_p)new_longp(1);
	if (w == NULL)
		return NULL;
	x = (long_p) long_add(v, w);
	pr_free(w);
	if (x == NULL)
		return NULL;
	x->dsize = -(x->dsize);
	return (long_p)x;
}

static long_p long_rshift(isp ist, long_p v, long_p w) {
	long_p a, b;
	long_p z = NULL;
	i64_t shiftby, newsize, wordshift, loshift, hishift;
	int i, j;
	digit lomask, himask;

	CONVERT_BINOP((long_p)v, (long_p)w, &a, &b);

	if (a->dsize < 0) {
		/* Right shifting negative numbers is harder */
		long_p a1, a2;
		a1 = (long_p) long_invert(ist, a);
		if (a1 == NULL)
			goto rshift_error;
		a2 = (long_p) long_rshift(ist, a1, b);
		pr_free(a1);
		if (a2 == NULL)
			goto rshift_error;
		z = (long_p) long_invert(ist, a2);
		pr_free(a2);
	}
	else {

		shiftby = longp2int64(ist, (long_p)b);
		if (shiftby < 0) {
			raise_exception(ist, OBJ(VALUE_EXC),
					"negative shift count");
			goto rshift_error;
		}
		wordshift = shiftby / SHIFT;
		newsize = abs(a->dsize) - wordshift;
		if (newsize <= 0) {
			z = new_longp_non_init(0);
			pr_free(a);
			pr_free(b);
			return (long_p)z;
		}
		loshift = shiftby % SHIFT;
		hishift = SHIFT - loshift;
		lomask = ((digit)1 << hishift) - 1;
		himask = MASK ^ lomask;
		z = new_longp_non_init((int)newsize);
		if (z == NULL)
			goto rshift_error;
		if (a->dsize < 0)
			z->dsize = -(z->dsize);
		for (i = 0, j = (int) wordshift; i < newsize; i++, j++) {
			z->digit[i] = (a->digit[j] >> loshift) & lomask;
			if (i+1 < newsize)
				z->digit[i] |=
				 (a->digit[j+1] << hishift) & himask;
		}
		z = long_normalize(z);
	}
rshift_error:
	pr_free(a);
	pr_free(b);
	return (long_p) z;

}

static long_p long_lshift(isp ist, long_p v, long_p w)
{
	/* This version due to Tim Peters */
	long_p a, b;
	long_p z = NULL;
	i64_t shiftby;
	int oldsize, newsize, wordshift, remshift, i, j;
	twodigits accum;

	CONVERT_BINOP(v, w, &a, &b);

	shiftby = longp2int64(ist, (long_p)b);
	if (shiftby < 0) {
		raise_exception(ist, OBJ(VALUE_EXC), "negative shift count");
		goto lshift_error;
	}
	if ((long)(int)shiftby != shiftby) {
		raise_exception(ist, OBJ(VALUE_EXC),
				"outrageous left shift count");
		goto lshift_error;
	}
	/* wordshift, remshift = divmod(shiftby, SHIFT) */
	wordshift = (int)shiftby / SHIFT;
	remshift = (int)shiftby - wordshift * SHIFT;

	oldsize = abs(a->dsize);
	newsize = oldsize + wordshift;
	if (remshift)
		++newsize;
	z = new_longp_non_init(newsize);
	if (z == NULL)
		goto lshift_error;
	if (a->dsize < 0)
		z->dsize = -(z->dsize);
	for (i = 0; i < wordshift; i++)
		z->digit[i] = 0;
	accum = 0;
	for (i = wordshift, j = 0; j < oldsize; i++, j++) {
		accum |= (twodigits)a->digit[j] << remshift;
		z->digit[i] = (digit)(accum & MASK);
		accum >>= SHIFT;
	}
	if (remshift)
		z->digit[newsize-1] = (digit)accum;
	else
		pr_assert(!accum);
	z = long_normalize(z);
lshift_error:
	pr_free(a);
	pr_free(b);
	return (long_p) z;
}


/* Bitwise and/xor/or operations */

static long_p long_bitwise(isp ist, long_p a, int op, /* '&', '|', '^' */ long_p b)
{
	digit maska, maskb; /* 0 or MASK */
	int negz;
	int size_a, size_b, size_z;
	long_p z;
	int i;
	digit diga, digb;
	long_p v;

	if (a->dsize < 0) {
		a = (long_p) long_invert(ist, a);
		maska = MASK;
	}
	else {
		Py_INCREF(a);
		maska = 0;
	}
	if (b->dsize < 0) {
		b = (long_p) long_invert(ist, b);
		maskb = MASK;
	}
	else {
		Py_INCREF(b);
		maskb = 0;
	}

	negz = 0;
	switch (op) {
	case '^':
		if (maska != maskb) {
			maska ^= MASK;
			negz = -1;
		}
		break;
	case '&':
		if (maska && maskb) {
			op = '|';
			maska ^= MASK;
			maskb ^= MASK;
			negz = -1;
		}
		break;
	case '|':
		if (maska || maskb) {
			op = '&';
			maska ^= MASK;
			maskb ^= MASK;
			negz = -1;
		}
		break;
	}

	/* JRH: The original logic here was to allocate the result value (z)
	  as the longer of the two operands. However, there are some cases
	  where the result is guaranteed to be shorter than that: AND of two
	  positives, OR of two negatives: use the shorter number. AND with
	  mixed signs: use the positive number. OR with mixed signs: use the
	  negative number. After the transformations above, op will be '&'
	  iff one of these cases applies, and mask will be non-0 for operands
	  whose length should be ignored.
	*/

	size_a = a->dsize;
	size_b = b->dsize;
	size_z = op == '&'
		? (maska
		  ? size_b
		  : (maskb ? size_a : min(size_a, size_b)))
		: max(size_a, size_b);
	z = new_longp_non_init(size_z);
	if (a == NULL || b == NULL || z == NULL) {
		Py_XDECREF(a);
		Py_XDECREF(b);
		Py_XDECREF(z);
		return NULL;
	}

	for (i = 0; i < size_z; ++i) {
		diga = (i < size_a ? a->digit[i] : 0) ^ maska;
		digb = (i < size_b ? b->digit[i] : 0) ^ maskb;
		switch (op) {
		case '&': z->digit[i] = diga & digb; break;
		case '|': z->digit[i] = diga | digb; break;
		case '^': z->digit[i] = diga ^ digb; break;
		}
	}
	z = long_normalize(z);
	if (negz == 0)
		return (long_p) z;
	v = long_invert(ist, z);
	pr_free(z);
	return v;
}

static long_p long_and(isp ist, long_p v, long_p w) {
	long_p a, b;
	long_p c;
	CONVERT_BINOP(v, w, &a, &b);
	c = long_bitwise(ist, a, '&', b);
	pr_free(a);
	pr_free(b);
	return c;
}

static long_p long_xor(isp ist, long_p v, long_p w)
{
	long_p a, b;
	long_p c;
	CONVERT_BINOP(v, w, &a, &b);
	c = long_bitwise(ist, a, '^', b);
	pr_free(a);
	pr_free(b);
	return c;
}

static long_p long_or(isp ist, long_p v, long_p w)
{
	long_p a, b;
	long_p c;
	CONVERT_BINOP(v, w, &a, &b);
	c = long_bitwise(ist, a, '|', b);
	pr_free(a);
	pr_free(b);
	return c;
}

long long_hash(long_p v)
{
	long x;
	int i, sign;

	/* This is designed so that ints and longs with the
	  same value hash to the same value, otherwise comparisons
	  of mapping keys will turn out weird */
	i = v->dsize;
	sign = 1;
	x = 0;
	if (i < 0) {
		sign = -1;
		i = -(i);
	}
#define LONG_BIT_SHIFT	(8*sizeof(long) - SHIFT)
	while (--i >= 0) {
		/* Force a native long #-bits (32 or 64) circular shift */
		x = ((x << SHIFT) & ~MASK) | ((x >> LONG_BIT_SHIFT) & MASK);
		x += v->digit[i];
	}
#undef LONG_BIT_SHIFT
	x = x * sign;
	if (x == -1)
		x = -2;
	return x;
}

i32_t int2i32t(isp ist, obj_p self) {
	if (self->data_type == DATA_TYPE_NONE) {					
		raise_exception(ist, OBJ(INTERNAL_EXC),					
			"object has no binary data, expected Integer");	
		return 0; }											
	if (!proto_owns_binary(ist, OBJ(INT_PROTO), self)) {				
		raise_exception(ist, OBJ(INTERNAL_EXC),					
			"object has wrong binary data, expected Integer");	
		return 0; }											
	if (self->data_type == DATA_TYPE_IMMDATA) {
		if ( self->data.i64 >  ((i64_t) 0x7fffffff) || 
			 self->data.i64 < -((i64_t) 0x80000000) ) {
			raise_exception(ist, OBJ(OVERFLOW_EXC),					
						         "integer too large to convert");	
			return 0;											
		}
		return (i32_t) self->data.i64;
	} else
		return longp2int32(ist, self->data.ptr);
}

i64_t int2i64t(isp ist, obj_p self) {
	if (self->data_type == DATA_TYPE_NONE) {					
		raise_exception(ist, OBJ(INTERNAL_EXC),					
			"object has no binary data, expected Integer");	
		return 0; }											
	if (!proto_owns_binary(ist, OBJ(INT_PROTO), self)) {				
		raise_exception(ist, OBJ(INTERNAL_EXC),					
			"object has wrong binary data, expected Integer");	
		return 0; }											
	if (self->data_type == DATA_TYPE_IMMDATA)
		return self->data.i64;
	else
		return longp2int64(ist, self->data.ptr);
}

obj_p chr_ch3(isp ist, obj_p self) {
	ch3_t ch3;
	i32_t val;
	val = int2i32t(ist, self);
	if (val < 0 || val >= MAX_UNICODE_VALUE) {
		raise_exception( ist, OBJ(VALUE_EXC), 
			             "integer value outside of unicode character range" );
		return NULL;
	}
	ch3.b0 = val       & 0xff;
	ch3.b1 = val >>  8 & 0xff;
	ch3.b2 = val >> 16 & 0xff;
	return NEW_STRING_CH3(&ch3, 1);
}

// ***************************** INT MODULE ***********************************

#define	INT_DATA_SIZE		8
#define is_Int(objid)		(has_proto(ist, objid, Int_OBJ))
#define int_imm_value(objid)	(objid->data.i64)

static obj_p SYM_LIMIT;	


MODULE_START(Int)
{
	Int_OBJ = OBJ(INT_PROTO);
	set_attr(ist, OBJ(OBJECT), sym(ist, "Int"), Int_OBJ);
	MODULE_SET_DOC(Int, "integer object prototype");
	set_obj_id(Int_OBJ, *, Int);
	SET_TYPE_IF_EXC(Int_OBJ, Int_OBJ, DATA_TYPE_IMMDATA) return;
	Int_OBJ->imm_data_len = IMM_DATA_LEN;
	Int_OBJ->data.i64   = 0;

	/* Dependent objects */
	OBJ(ZERO_INT)		= NEW_INT(0);
}

DEF(Int, init_, FORM_STAR_PARAM) {
	obj_p i = NULL;
	int llen = (int) list_len(ist, parms[1]);
	BIN_EMPTY_CHK();
	if (llen == 1) {
		INT____PARAM(0, i);
	} else if (llen > 1) {
		raise_exception(ist, OBJ(TYPE_EXC), "incorrect number of parameters");
		return NULL;
	} else {
		SET_TYPE_IF_EXC(Int_OBJ, self, DATA_TYPE_IMMDATA) return NULL;
		self->imm_data_len = 8;
		self->data.i64 = 0;
		return OBJ(NONE);
	}
	if (i->data_type == DATA_TYPE_IMMDATA) {
		SET_TYPE_IF_EXC(Int_OBJ, self, DATA_TYPE_IMMDATA) return NULL;
		self->imm_data_len = 8;
		self->data.i64 = i->data.i64;
	} else {
		SET_TYPE_IF_EXC(Int_OBJ, self, DATA_TYPE_DATAPTR) return NULL;
		self->data.ptr  = copy_longp(i->data.ptr, 0);
	}
	return OBJ(NONE);
}

DEF(Int, str_, FPARM1(spec, NEW_STRING(""))) {
	obj_p res, self_str;
	char *spec, *wid_p = NULL, *prec_p = NULL, *p;
	int neg_flag;
	int left_align = FALSE;
	int hdr_flag   = FALSE;
	int blnk_flag  = FALSE;
	int pos_flag   = FALSE;
	int chr_flag   = FALSE;
	int oct_flag   = FALSE;
	int hex_flag   = FALSE;
	int heX_flag   = FALSE;
	int i, res_len, self_len, txt_len, width = 0, precision = 0;
	ch3_p res_ptr, self_ptr, ptr;
	BIN_STR_CHK(Int);
	STRING_PARAM(1, spec);
	for(p=spec; *p; p++) {
		if      (!wid_p && !prec_p && *p == '-') left_align = TRUE;
		else if (!wid_p && !prec_p && *p == '#')   hdr_flag = TRUE;
		else if (!wid_p && !prec_p && *p == ' ')  blnk_flag = TRUE;
		else if (!wid_p && !prec_p && *p == '+')   pos_flag = TRUE;
		else if (!wid_p && !prec_p && *p > '0' && *p <= '9') wid_p = p;
		else if (!prec_p && *p == '.') { prec_p = p+1; *p = 0; }
		else if ( *p == 'd' || *p == 'D' || *p == 'i' || *p == 'I' || 
			      *p == 'c' || *p == 'C' || *p == 'o' || *p == 'O' || 
				  *p == 'x' || *p == 'X' ) {
			if (*(p+1) != 0) {
				raise_exception( ist, OBJ(TYPE_EXC), 
					             "extra char(s) found at end of fmt spec: \"%s\"", spec );
				return NULL;
			}
			if (*p == 'c' || *p == 'C') chr_flag = TRUE;
			else if (*p == 'o' || *p == 'O') oct_flag = TRUE;
			else if (*p == 'x') hex_flag = TRUE;	
			else if (*p == 'X') heX_flag = TRUE;
			*p = 0; 
			break; 
		} else if (*p < '0' || *p > '9') {
			raise_exception( ist, OBJ(VALUE_EXC), 
				             "invalid fmt spec in str_ parameter: \"%s\"", spec );
			return NULL;
		}
	}
	if (chr_flag) {
		self_str = chr_ch3(ist, self);
	} else if (self->data_type == DATA_TYPE_IMMDATA) {
		char str[32], *p, *pend;
		i64_t val = int2i64t(ist, self);
		neg_flag = (val < 0);
		if (oct_flag) {
			long_p longp = new_longp(val);
			self_str = long_format(longp, 8, FALSE);
			pr_free(longp);
			goto gotstr;
		}
		str[0] = '-';
		if (neg_flag) val = -val;
		if (hex_flag || heX_flag)
			apr_snprintf(str+1, sizeof(str)-1, "%"APR_UINT64_T_HEX_FMT, val);
		else
			apr_snprintf(str+1, sizeof(str)-1, "%"APR_INT64_T_FMT, val);
		if (heX_flag) {
			pend = str + strlen(str);
			for (p=str; *p; p++)
				*p = toupper(*p);
		}
		self_str = NEW_STRING(neg_flag ? str : str+1);
	} else {
		int base = ((hex_flag|heX_flag) ? 16 : (oct_flag ? 8 : 10));
		self_str = long_format(self->data.ptr, base, heX_flag);
	}
gotstr:
	if (wid_p)  width     = atoi(wid_p);
	if (prec_p) precision = atoi(prec_p);
	self_ptr = pr_ch3ptr(self_str);
	self_len = pr_strlen(self_str);
	if (self_ptr->b0 == (u8_t) '-') {
		self_ptr++;
		self_len--;
		blnk_flag = FALSE;
		pos_flag  = FALSE;
		neg_flag  = TRUE;
	} else
		neg_flag = FALSE;
	if (pos_flag) blnk_flag = FALSE;
	if (!oct_flag && !hex_flag && !heX_flag) hdr_flag = FALSE;
	if (prec_p)
		txt_len = max(self_len, precision);
	else
		txt_len = self_len;
	if (pos_flag || neg_flag || blnk_flag) txt_len++;
	if (hdr_flag) txt_len += 2;
	if (wid_p)
		res_len = max(txt_len, width);
	else
		res_len = txt_len;
	ptr = res_ptr = pr_malloc(res_len*3);
	if (!left_align)
		for (i=0; i < res_len-txt_len; i++, ptr++) {
			ptr->b0 = (u8_t) ' '; ptr->b1 = 0;  ptr->b2 = 0;
		}
	if (pos_flag)  { ptr->b0 = (u8_t) '+';  ptr->b1 = 0;  ptr->b2 = 0;  ptr++; }
	if (neg_flag)  { ptr->b0 = (u8_t) '-';  ptr->b1 = 0;  ptr->b2 = 0;  ptr++; }
	if (blnk_flag) { ptr->b0 = (u8_t) ' ';  ptr->b1 = 0;  ptr->b2 = 0;  ptr++; }
	if (hdr_flag & oct_flag) {
		ptr->b0 = (u8_t) '0';  ptr->b1 = 0;  ptr->b2 = 0;  ptr++;
		ptr->b0 = (u8_t) 'o';  ptr->b1 = 0;  ptr->b2 = 0;  ptr++;
	}
	if (hdr_flag & (hex_flag|heX_flag)) {
		ptr->b0 = (u8_t) '0';  ptr->b1 = 0;  ptr->b2 = 0;  ptr++;
		ptr->b0 = (u8_t) 'x';  ptr->b1 = 0;  ptr->b2 = 0;  ptr++;
	}
	if (prec_p)
		for (i=0; i < (precision-self_len); i++, ptr++) {
			ptr->b0 = (u8_t) '0'; ptr->b1 = 0; ptr->b2 = 0; 
		}
	for (i=0; i < self_len; i++, ptr++, self_ptr++) {
		ptr->b0 = self_ptr->b0;
		ptr->b1 = self_ptr->b1;
		ptr->b2 = self_ptr->b2;
	}
	if (left_align)
		for (i=0; i < res_len-txt_len; i++, ptr++) {
			ptr->b0 = (u8_t) ' '; ptr->b1 = 0;  ptr->b2 = 0;
		}
	res = new_string_ch3_obj(ist, res_ptr, res_len);
	pr_free(res_ptr);
	return res;
}

DEF(Int, bool__QUES, NULL) {
	BIN_CONTENT_CHK(Int);
	if (self->data_type == DATA_TYPE_IMMDATA) {
		if (int_imm_value(self)) return OBJ(PR_TRUE);
		else				 return OBJ(PR_FALSE);
	} else {
		if (long_dsize(self)) return OBJ(PR_TRUE);
		else				  return OBJ(PR_FALSE);
	}
}

DEF(Int, hash_, NULL) { 
	long_p v;
	obj_p res;
	BIN_CONTENT_CHK(Int);
	if (self->data_type == DATA_TYPE_IMMDATA) {
		res = new_hash_obj(ist, long_hash(v = new_longp(int_imm_value(self))));
		pr_free(v);
		return res;
	}
	else
		return new_hash_obj(ist, long_hash(self->data.ptr));
}

DEF(Int, abs_, NULL){
	BIN_CONTENT_CHK(Int);
	if (self->data_type == DATA_TYPE_IMMDATA)
		return NEW_INT(abs(int_imm_value(self)));
	else {
		obj_p res = copy_object(ist, self);
		long_dsize(res) = abs(long_dsize(res));
		return res;
	}
}

DEF(Int, neg_, NULL){
	BIN_CONTENT_CHK(Int);
	if (self->data_type == DATA_TYPE_IMMDATA)
		return NEW_INT( - int_imm_value(self) );
	else {
		obj_p res = copy_object(ist, self);
		long_dsize(res) = -long_dsize(res);
		return res;
	}
}

DEF(Int, pos_, NULL){
	BIN_CONTENT_CHK(Int);
	return self;
}

DEF(Int, add_, FORM_RPARAM){
	obj_p other = parms[1];
	i64_t siv;
	i64_t oiv;

	BIN_CONTENT_CHK(Int);
	if (!is_Int(other)) {
		if (covers(other, self))
			return call_func1(ist, other, SYM(ADD_), self);
		raise_exception(ist, OBJ(TYPE_EXC), "Integer cannot be added to this object");
		return OBJ(NONE);
	}
	siv = int_imm_value(self);
	oiv = int_imm_value(other);
	if ( self->data_type  == DATA_TYPE_IMMDATA &&  
		 other->data_type == DATA_TYPE_IMMDATA &&
		 (siv < MAX64 || oiv < MAX64)          &&
		 (siv > MIN64 || oiv > MIN64)          )
		return NEW_INT(siv+oiv);
	else {
		long_p a, b;
		if (self->data_type  == DATA_TYPE_IMMDATA)
			a = new_longp(siv);
		else
			a = self->data.ptr;
		if (other->data_type  == DATA_TYPE_IMMDATA)
			b = new_longp(oiv);
		else
			b = other->data.ptr;
		return new_int_long_obj(ist, long_add(a,b));
	}
}

DEF(Int, div_, FORM_RPARAM){
	obj_p res;
	obj_p float_self, float_other, other = parms[1];
	BIN_CONTENT_CHK(Int);
	if (!is_Int(other)) {
		if (covers(other, self))
			return call_func1(ist, other, SYM(RDIV_), self);
		raise_exception(ist, OBJ(TYPE_EXC), "Integer cannot be divided by this object");
		return OBJ(NONE);
	}
	float_self  = call_func1(ist, OBJ(FLOAT_PROTO), SYM(COERCE_), self); if_exc_return NULL;
	float_other = call_func1(ist, OBJ(FLOAT_PROTO), SYM(COERCE_), other); if_exc_return NULL;
	res = call_func1(ist, float_self, SYM(DIV_), float_other);
	del_unlock(float_self); del_unlock(float_other); 
	return res;
}

DEF(Int, floorDiv_, FORM_RPARAM){
	obj_p other = parms[1];
	BIN_CONTENT_CHK(Int);
	if (!is_Int(other)) {
		if (covers(other, self))
			return call_func1(ist, other, SYM(RFLOORDIV_), self);
		raise_exception(ist, OBJ(TYPE_EXC), "Integer cannot be floor divided by this object");
		return OBJ(NONE);
	}
	if ( self->data_type  == DATA_TYPE_IMMDATA &&  
		 other->data_type == DATA_TYPE_IMMDATA ) {
		if (int_imm_value(other) == 0) {
			raise_exception(ist, OBJ(DIVIDEZERO_EXC), NULL);
			return NULL;
		}
		return NEW_INT(int_imm_value(self) / int_imm_value(other));
	} else {
		long_p a, b;
		if (self->data_type  == DATA_TYPE_IMMDATA)
			a = new_longp(self->data.i64);
		else
			a = self->data.ptr;
		if (other->data_type  == DATA_TYPE_IMMDATA)
			b = new_longp(other->data.i64);
		else
			b = other->data.ptr;
		return new_int_long_obj(ist, long_div(a,b));
	}
}

DEF(Int, mod_, FORM_RPARAM) {
	obj_p other = parms[1];
	BIN_CONTENT_CHK(Int);
	if (!is_Int(other)) {
		if (covers(other, self))
			return call_func1(ist, other, SYM(RMOD_), self);
		raise_exception(ist, OBJ(TYPE_EXC), "Integer cannot be modded by this object");
		return OBJ(NONE);
	}
	if ( self->data_type  == DATA_TYPE_IMMDATA &&  
		 other->data_type == DATA_TYPE_IMMDATA ) {
		if (int_imm_value(other) == 0) {
			raise_exception(ist, OBJ(DIVIDEZERO_EXC), "modulo by zero");
			return NULL;
		}
		return NEW_INT(int_imm_value(self) % int_imm_value(other));
	} else {
		long_p a, b;
		if (self->data_type  == DATA_TYPE_IMMDATA)
			a = new_longp(self->data.i64);
		else
			a = self->data.ptr;
		if (other->data_type  == DATA_TYPE_IMMDATA)
			b = new_longp(other->data.i64);
		else
			b = other->data.ptr;
		return new_int_long_obj(ist, long_mod(ist, a,b));
	}
}

DEF(Int, mul_, FORM_RPARAM){
	obj_p other = parms[1];
	i64_t siv;
	i64_t oiv;

	BIN_CONTENT_CHK(Int);
	if (!is_Int(other)) {
		if (covers(other, self))
			return call_func1(ist, other, SYM(MUL_), self);
		raise_exception(ist, OBJ(TYPE_EXC), "Integer cannot be multiplied by this object");
		return OBJ(NONE);
	}
	siv = int_imm_value(self);
	oiv = int_imm_value(other);
	if ( self->data_type  == DATA_TYPE_IMMDATA &&  
		 other->data_type == DATA_TYPE_IMMDATA &&
		 siv < MAX32 && oiv < MAX32 && siv > MIN32 && oiv > MIN32 )
		return NEW_INT(siv*oiv);
	else {
		long_p a, b;
		if (self->data_type  == DATA_TYPE_IMMDATA)
			a = new_longp(siv);
		else
			a = self->data.ptr;
		if (other->data_type  == DATA_TYPE_IMMDATA)
			b = new_longp(oiv);
		else
			b = other->data.ptr;
		return new_int_long_obj(ist, long_mul(a,b));
	}
}

DEF(Int, sub_, FORM_RPARAM){
	obj_p other = parms[1];
	i64_t siv;
	i64_t oiv;

	BIN_CONTENT_CHK(Int);
	if (!is_Int(other)) {
		if (covers(other, self))
			return call_func1(ist, other, SYM(RSUB_), self);
		raise_exception(ist, OBJ(TYPE_EXC), "Integer cannot be subtracted from this object");
		return OBJ(NONE);
	}
	siv = int_imm_value(self);
	oiv = int_imm_value(other);
	if ( self->data_type  == DATA_TYPE_IMMDATA &&  
		 other->data_type == DATA_TYPE_IMMDATA &&
		 siv < MAX64 && oiv < MAX64 && siv > MIN64 && oiv > MIN64 )
		return NEW_INT(siv-oiv);
	else {
		long_p a, b;
		if (self->data_type  == DATA_TYPE_IMMDATA)
			a = new_longp(siv);
		else
			a = self->data.ptr;
		if (other->data_type  == DATA_TYPE_IMMDATA)
			b = new_longp(oiv);
		else
			b = other->data.ptr;
		return new_int_long_obj(ist, long_sub(a,b));
	}
}

DEF(Int, pow_, FORM_RPARAM){
	obj_p res, other = parms[1], float_self, float_other;
	long_p a, b;
	int iso = 0;

	BIN_CONTENT_CHK(Int);
	if (!is_Int(other) || (iso = int_sign(other)) < 0) {
		float_self = call_func1(ist, OBJ(FLOAT_PROTO), SYM(COERCE_), self); if_exc_return NULL;
		float_other = call_func1(ist, OBJ(FLOAT_PROTO), SYM(COERCE_), other); if_exc_return NULL;
		res = call_func1(ist, float_self, SYM(POW_), float_other);
		del_unlock(float_self); del_unlock(float_other); 
		return res;
	}
	if (!iso) return NEW_INT(1);

	if (self->data_type  == DATA_TYPE_IMMDATA)
		a = new_longp(self->data.i64);
	else
		a = self->data.ptr;
	if (other->data_type  == DATA_TYPE_IMMDATA)
		b = new_longp(other->data.i64);
	else
		b = other->data.ptr;
	return new_int_long_obj(ist, long_pow(ist, a, b, NULL));
}

#define ORD(ch3p) ((((((u32_t)(ch3p)->b2)<<8)|((ch3p)->b1))<<8)|((ch3p)->b0))

DEF(Int, cmp_, FORM_RPARAM){
	obj_p other = parms[1];
	BIN_CONTENT_CHK(Int);
	if (has_proto(ist, other, OBJ(STRING_PROTO)) && pr_strlen(other) == 1)
		other = NEW_INT(ORD(pr_ch3ptr(other)));
	if (!is_Int(other)) {
		if (covers(other, self))
			return call_func1(ist, other, SYM(RCMP_), self);
		raise_exception(ist, OBJ(TYPE_EXC), "This object cannot be compared to an Integer");
		return OBJ(NONE);
	}
	if ( self->data_type  == DATA_TYPE_IMMDATA &&  
		 other->data_type == DATA_TYPE_IMMDATA ) {
		if (int_imm_value(self) == int_imm_value(other))
			return NEW_INT(0);
		else if (int_imm_value(self) > int_imm_value(other))
			return NEW_INT(1);
		else
			return NEW_INT(-1);
	} else {
		long_p a, b;
		if (self->data_type  == DATA_TYPE_IMMDATA)
			a = new_longp(self->data.i64);
		else
			a = self->data.ptr;
		if (other->data_type  == DATA_TYPE_IMMDATA)
			b = new_longp(other->data.i64);
		else
			b = other->data.ptr;
		return NEW_INT(long_compare(a,b));
	}
}

DEF(Int, eq__QUES, FORM_RPARAM){
	obj_p other = parms[1];
	BIN_CONTENT_CHK(Int);
	if (has_proto(ist, other, OBJ(STRING_PROTO)) && pr_strlen(other) == 1) {
		char* s = pr2c_strptr(ist, parms[1]); if_exc_return NULL;
		other = NEW_INT(s[0]);
		pr_free(s);
	}
	if (!is_Int(other)) {
		int c = covers(other, self);
		if (catch_exception(ist, OBJ(FUNCNOTFOUND_EXC), NULL)) 
			return OBJ(PR_FALSE);
		if (c) return call_func1(ist, other, SYM(EQ__QUES), self);
		return OBJ(PR_FALSE);
	}
	if ( self->data_type  == DATA_TYPE_IMMDATA &&  
		 other->data_type == DATA_TYPE_IMMDATA ) {
		if (int_imm_value(self) == int_imm_value(other)) return OBJ(PR_TRUE);
		else                                             return OBJ(PR_FALSE);
	} else {
		long_p a, b;
		if (self->data_type  == DATA_TYPE_IMMDATA)
			a = new_longp(self->data.i64);
		else
			a = self->data.ptr;
		if (other->data_type  == DATA_TYPE_IMMDATA)
			b = new_longp(other->data.i64);
		else
			b = other->data.ptr;
		if (long_compare(a,b)) return OBJ(PR_FALSE);
		else                   return OBJ(PR_TRUE);
	}
}
DEF(Int, invert_, NULL){
	BIN_CONTENT_CHK(Int);
	if ( self->data_type  == DATA_TYPE_IMMDATA)
		return NEW_INT( ~ int_imm_value(self) );
	else
		return new_int_long_obj(ist, long_invert(ist, self->data.ptr));
}

DEF(Int, xor_, FORM_RPARAM) {
	obj_p other = parms[1];
	BIN_CONTENT_CHK(Int);
	if (!is_Int(other)) {
		raise_exception(ist, OBJ(TYPE_EXC), "object cannot be xor'd with an Integer");
		return OBJ(NONE);
	}
	if ( self->data_type  == DATA_TYPE_IMMDATA &&  
		 other->data_type == DATA_TYPE_IMMDATA ) {
		return NEW_INT(int_imm_value(self) ^ int_imm_value(other));
	} else {
		long_p a, b;
		if (self->data_type  == DATA_TYPE_IMMDATA)
			a = new_longp(self->data.i64);
		else
			a = self->data.ptr;
		if (other->data_type  == DATA_TYPE_IMMDATA)
			b = new_longp(other->data.i64);
		else
			b = other->data.ptr;
		return new_int_long_obj(ist, long_xor(ist, a, b));
	}
}

DEF(Int, and_, FORM_RPARAM) {
	obj_p other = parms[1];
	BIN_CONTENT_CHK(Int);
	if (!is_Int(other)) {
		raise_exception(ist, OBJ(TYPE_EXC), "object cannot be and'd with an Integer");
		return OBJ(NONE);
	}
	if ( self->data_type  == DATA_TYPE_IMMDATA &&  
		 other->data_type == DATA_TYPE_IMMDATA ) {
		return NEW_INT(int_imm_value(self) & int_imm_value(other));
	} else {
		long_p a, b;
		if (self->data_type  == DATA_TYPE_IMMDATA)
			a = new_longp(self->data.i64);
		else
			a = self->data.ptr;
		if (other->data_type  == DATA_TYPE_IMMDATA)
			b = new_longp(other->data.i64);
		else
			b = other->data.ptr;
		return new_int_long_obj(ist, long_and(ist, a, b));
	}
}

DEF(Int, or_, FORM_RPARAM) {
	obj_p other = parms[1];
	BIN_CONTENT_CHK(Int);
	if (!is_Int(other)) {
		raise_exception(ist, OBJ(TYPE_EXC), "object cannot be or'd with an Integer");
		return OBJ(NONE);
	}
	if ( self->data_type  == DATA_TYPE_IMMDATA &&  
		 other->data_type == DATA_TYPE_IMMDATA ) {
		return NEW_INT(int_imm_value(self) | int_imm_value(other));
	} else {
		long_p a, b;
		if (self->data_type  == DATA_TYPE_IMMDATA)
			a = new_longp(self->data.i64);
		else
			a = self->data.ptr;
		if (other->data_type  == DATA_TYPE_IMMDATA)
			b = new_longp(other->data.i64);
		else
			b = other->data.ptr;
		return new_int_long_obj(ist, long_or(ist, a, b));
	}
}

DEF(Int, rShift_, FORM_RPARAM) {
	obj_p other = parms[1];
	i64_t siv, oiv;
	BIN_CONTENT_CHK(Int);
	if (!is_Int(other)) {
		raise_exception(ist, OBJ(TYPE_EXC), "object cannot be shifted with an Integer");
		return OBJ(NONE);
	}
	siv = int_imm_value(self);
	oiv = int_imm_value(other);
	if ( self->data_type  == DATA_TYPE_IMMDATA &&  
		 other->data_type == DATA_TYPE_IMMDATA &&
		 siv < MAX32 &&  siv > MIN32 && oiv > -32 )
		return NEW_INT(int_imm_value(self) >> int_imm_value(other));
	else {
		long_p a, b;
		if (self->data_type  == DATA_TYPE_IMMDATA)
			a = new_longp(self->data.i64);
		else
			a = self->data.ptr;
		if (other->data_type  == DATA_TYPE_IMMDATA)
			b = new_longp(other->data.i64);
		else
			b = other->data.ptr;
		return new_int_long_obj(ist, long_rshift(ist, a, b));
	}
}

DEF(Int, lShift_, FORM_RPARAM) {
	obj_p other = parms[1];
	i64_t siv, oiv;
	BIN_CONTENT_CHK(Int);
	if (!is_Int(other)) {
		raise_exception(ist, OBJ(TYPE_EXC), "object cannot be shifted with an Integer");
		return OBJ(NONE);
	}
	siv = int_imm_value(self);
	oiv = int_imm_value(other);
	if ( self->data_type  == DATA_TYPE_IMMDATA &&  
		 other->data_type == DATA_TYPE_IMMDATA &&
		 siv < MAX32 &&  siv > MIN32 && oiv < 32 )
		return NEW_INT(int_imm_value(self) << int_imm_value(other));
	else {
		long_p a, b;
		if (self->data_type  == DATA_TYPE_IMMDATA)
			a = new_longp(self->data.i64);
		else
			a = self->data.ptr;
		if (other->data_type  == DATA_TYPE_IMMDATA)
			b = new_longp(other->data.i64);
		else
			b = other->data.ptr;
		return new_int_long_obj(ist, long_lshift(ist, a, b));
	}
}

DEF(Int, iter_, NULL) {
	obj_p gen_obj = NEW_INT(0);
	BIN_CONTENT_CHK(Int);
	gen_obj->immutable = FALSE;
	gen_obj->attr_proto.proto = IntGen_OBJ;
	set_attr(ist, gen_obj, SYM_LIMIT, self);
	return gen_obj;
}

DEF(Int, chr_, NULL) {
	BIN_CONTENT_CHK(Int);
	return chr_ch3(ist, self);
}

MODULE_START(IntGen)
{
	IntGen_OBJ = NEW_OBJ(OBJ(ITER_PROTO));
	MODULE_SET_DOC(IntGen, "number generator object prototype");
	set_obj_id(IntGen_OBJ, *, IntGen);
}

DEF(IntGen, next, NULL) {
	obj_p limit, tmp, res;
	BIN_CONTENT_CHK(IntGen);
	if ( !(limit=get_attr(ist, self, SYM_LIMIT)) ||
		 (  self->data_type == DATA_TYPE_IMMDATA && 
		   limit->data_type == DATA_TYPE_IMMDATA ?
			 (self->data.i64 == limit->data.i64) :
			 !(call_func1(ist, self, SYM(CMP_), limit)->data.i64) ) ) {
		if (limit) {
			read_unlock(ist, self);
			del_attr(ist, self, SYM_LIMIT);
			rdlock_rtrn(self) NULL;
		}
		raise_exception(ist, OBJ(STOP_ITERATION_EXC), NULL);
		return NULL;
	}
	if (self->data_type == DATA_TYPE_IMMDATA)
		res = NEW_INT(self->data.i64);
	else
		res = new_int_long_obj(ist, self->data.ptr);
	if (self->data_type == DATA_TYPE_IMMDATA) {
		def_write_lock(self);
		int_imm_value(self)++;
		def_write_unlock(self);
	} else {
		call_func1(ist, self, SYM(CMP_), tmp = NEW_INT(1));
		del_unlock(tmp);
	}
	return res;
}

DEF(Int, objList_, FORM_RPARAM) {
	return parms[1];
}

MAIN_MODULE_INIT(Int)
{
	SYM_LIMIT = sym(ist, "limit");

	MODULE_SUB_INIT(Int);
	MODULE_ADD_SYM(Int, init_);
	MODULE_ADD_SYM(Int, bool__QUES);
	MODULE_ADD_SYM(Int, str_);
	MODULE_ADD_SYM(Int, hash_);
	MODULE_ADD_SYM(Int, eq__QUES);
	MODULE_ADD_SYM(Int, cmp_);
	MODULE_ADD_SYM(Int, chr_);
	MODULE_ADD_SYM(Int, abs_);
	MODULE_ADD_SYM(Int, neg_);
	MODULE_ADD_SYM(Int, pos_);
	MODULE_ADD_SYM(Int, add_);
	MODULE_ADD_SYM(Int, sub_);
	MODULE_ADD_SYM(Int, mul_);
	MODULE_ADD_SYM(Int, div_);
	MODULE_ADD_SYM(Int, floorDiv_);
	MODULE_ADD_SYM(Int, mod_);
	MODULE_ADD_SYM(Int, pow_);
	MODULE_ADD_SYM(Int, invert_);
	MODULE_ADD_SYM(Int, xor_);
	MODULE_ADD_SYM(Int, and_);
	MODULE_ADD_SYM(Int, or_);
	MODULE_ADD_SYM(Int, rShift_);
	MODULE_ADD_SYM(Int, lShift_);
	MODULE_ADD_SYM(Int, iter_);
	MODULE_ADD_SYM(Int, objList_);

	MODULE_SUB_INIT(IntGen);
	MODULE_ADD_SYM(IntGen, next);

	check_exceptions(ist);
}
