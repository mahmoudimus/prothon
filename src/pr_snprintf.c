/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2003 The Apache Software Foundation.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:
 *       "This product includes software developed by the
 *        Apache Software Foundation (http://www.apache.org/)."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "Apache" and "Apache Software Foundation" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache",
 *    nor may "Apache" appear in their name, without prior written
 *    permission of the Apache Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE APACHE SOFTWARE FOUNDATION OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Software Foundation.  For more
 * information on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 *
 * The apr_vsnprintf/apr_snprintf functions are based on, and used with the
 * permission of, the  SIO stdio-replacement strx_* functions by Panos
 * Tsirigotis <panos@alumni.cs.colorado.edu> for xinetd.
 */

#include <apr.h>
#include <apr_strings.h>
#include <apr_portable.h>
#include <apr_lib.h>

#include <math.h>

#include "pr_snprintf.h"

#define NUL '\0'
#define WIDE_INT long

typedef WIDE_INT wide_int;
typedef unsigned WIDE_INT u_wide_int;
typedef apr_int64_t widest_int;
#ifdef __TANDEM
/* Although Tandem supports "long long" there is no unsigned variant. */
typedef unsigned long       u_widest_int;
#else
typedef apr_uint64_t u_widest_int;
#endif
typedef int bool_int;

#define S_NULL "(null)"
#define S_NULL_LEN 6

#define FLOAT_DIGITS 6
#define EXPONENT_LENGTH 10

/*
 * NUM_BUF_SIZE is the size of the buffer used for arithmetic conversions
 *
 * NOTICE: this is a magic number; do not decrease it
 */
#define NUM_BUF_SIZE 512

/*
 * cvt.c - IEEE floating point formatting routines for FreeBSD
 * from GNU libc-4.6.27.  Modified to be thread safe.
 */

/*
 *    apr_ecvt converts to decimal
 *      the number of digits is specified by ndigit
 *      decpt is set to the position of the decimal point
 *      sign is set to 0 for positive, 1 for negative
 */

#define NDIG 80

/* buf must have at least NDIG bytes */
static char *apr_cvt(double arg, int ndigits, int *decpt, int *sign, 
                     int eflag, char *buf)
{
    register int r2;
    double fi, fj;
    register char *p, *p1;
    
    if (ndigits >= NDIG - 1)
        ndigits = NDIG - 2;
    r2 = 0;
    *sign = 0;
    p = &buf[0];
    if (arg < 0) {
        *sign = 1;
        arg = -arg;
    }
    arg = modf(arg, &fi);
    p1 = &buf[NDIG];
    /*
     * Do integer part
     */
    if (fi != 0) {
        p1 = &buf[NDIG];
        while (p1 > &buf[0] && fi != 0) {
            fj = modf(fi / 10, &fi);
            *--p1 = (int) ((fj + .03) * 10) + '0';
            r2++;
        }
        while (p1 < &buf[NDIG])
            *p++ = *p1++;
    }
    else if (arg > 0) {
        while ((fj = arg * 10) < 1) {
            arg = fj;
            r2--;
        }
    }
    p1 = &buf[ndigits];
    if (eflag == 0)
        p1 += r2;
    *decpt = r2;
    if (p1 < &buf[0]) {
        buf[0] = '\0';
        return (buf);
    }
    while (p <= p1 && p < &buf[NDIG]) {
        arg *= 10;
        arg = modf(arg, &fj);
        *p++ = (int) fj + '0';
    }
    if (p1 >= &buf[NDIG]) {
        buf[NDIG - 1] = '\0';
        return (buf);
    }
    p = p1;
    *p1 += 5;
    while (*p1 > '9') {
        *p1 = '0';
        if (p1 > buf)
            ++ * --p1;
        else {
            *p1 = '1';
            (*decpt)++;
            if (eflag == 0) {
                if (p > buf)
                    *p = '0';
                p++;
            }
        }
    }
    *p = '\0';
    return (buf);
}

static char *apr_ecvt(double arg, int ndigits, int *decpt, int *sign, char *buf)
{
    return (apr_cvt(arg, ndigits, decpt, sign, 1, buf));
}

static char *apr_fcvt(double arg, int ndigits, int *decpt, int *sign, char *buf)
{
    return (apr_cvt(arg, ndigits, decpt, sign, 0, buf));
}

/*
 * apr_gcvt  - Floating output conversion to
 * minimal length string
 */

static char *apr_gcvt(double number, int ndigit, char *buf, int altform)
{
    int sign, decpt;
    register char *p1, *p2;
    register int i;
    char buf1[NDIG];

    p1 = apr_ecvt(number, ndigit, &decpt, &sign, buf1);
    p2 = buf;
    if (sign)
        *p2++ = '-';
    for (i = ndigit - 1; i > 0 && p1[i] == '0'; i--)
        ndigit--;
    if ((decpt >= 0 && decpt - ndigit > 4)
        || (decpt < 0 && decpt < -3)) {                /* use E-style */
        decpt--;
        *p2++ = *p1++;
        *p2++ = '.';
        for (i = 1; i < ndigit; i++)
            *p2++ = *p1++;
        *p2++ = 'e';
        if (decpt < 0) {
            decpt = -decpt;
            *p2++ = '-';
        }
        else
            *p2++ = '+';
        if (decpt / 100 > 0)
            *p2++ = decpt / 100 + '0';
        if (decpt / 10 > 0)
            *p2++ = (decpt % 100) / 10 + '0';
        *p2++ = decpt % 10 + '0';
    }
    else {
        if (decpt <= 0) {
            if (*p1 != '0')
                *p2++ = '.';
            while (decpt < 0) {
                decpt++;
                *p2++ = '0';
            }
        }
        for (i = 1; i <= ndigit; i++) {
            *p2++ = *p1++;
            if (i == decpt)
                *p2++ = '.';
        }
        if (ndigit < decpt) {
            while (ndigit++ < decpt)
                *p2++ = '0';
            *p2++ = '.';
        }
    }
    if (p2[-1] == '.' && !altform)
        p2--;
    *p2 = '\0';
    return (buf);
}

/*
 * The INS_CHAR macro inserts a character in the buffer and writes
 * the buffer back to disk if necessary
 * It uses the char pointers sp and bep:
 *      sp points to the next available character in the buffer
 *      bep points to the end-of-buffer+1
 * While using this macro, note that the nextb pointer is NOT updated.
 *
 * NOTE: Evaluation of the c argument should not have any side-effects
 */
#define INS_CHAR(c, sp, bep, cc)                    \
{                                                   \
    if (sp) {                                       \
        if (sp >= bep)                              \
            return -1;                              \
        *sp++ = (c);                                \
    }                                               \
    cc++;                                           \
}

#define NUM(c) (c - '0')

#define STR_TO_DEC(str, num)                        \
    num = NUM(*str++);                              \
    while (apr_isdigit(*str))                       \
    {                                               \
        num *= 10 ;                                 \
        num += NUM(*str++);                         \
    }

/*
 * This macro does zero padding so that the precision
 * requirement is satisfied. The padding is done by
 * adding '0's to the left of the string that is going
 * to be printed. We don't allow precision to be large
 * enough that we continue past the start of s.
 *
 * NOTE: this makes use of the magic info that s is
 * always based on num_buf with a size of NUM_BUF_SIZE.
 */
#define FIX_PRECISION(adjust, precision, s, s_len)  \
    if (adjust) {                                   \
        int p = precision < NUM_BUF_SIZE - 1 ? precision : NUM_BUF_SIZE - 1; \
        while (s_len < p)                           \
        {                                           \
            *--s = '0';                             \
            s_len++;                                \
        }                                           \
    }

/*
 * Macro that does padding. The padding is done by printing
 * the character ch.
 */
#define PAD(width, len, ch)                         \
do                                                  \
{                                                   \
    INS_CHAR(ch, sp, bep, cc);                      \
    width--;                                        \
}                                                   \
while (width > len)

/*
 * Convert num to its decimal format.
 * Return value:
 *   - a pointer to a string containing the number (no sign)
 *   - len contains the length of the string
 *   - is_negative is set to TRUE or FALSE depending on the sign
 *     of the number (always set to FALSE if is_unsigned is TRUE)
 *
 * The caller provides a buffer for the string: that is the buf_end argument
 * which is a pointer to the END of the buffer + 1 (i.e. if the buffer
 * is declared as buf[ 100 ], buf_end should be &buf[ 100 ])
 *
 * Note: we have 2 versions. One is used when we need to use quads
 * (conv_10_quad), the other when we don't (conv_10). We're assuming the
 * latter is faster.
 */
static char *conv_10(register wide_int num, register bool_int is_unsigned,
                     register bool_int *is_negative, char *buf_end,
                     register int *len)
{
    register char *p = buf_end;
    register u_wide_int magnitude;

    if (is_unsigned) {
        magnitude = (u_wide_int) num;
        *is_negative = FALSE;
    }
    else {
        *is_negative = (num < 0);

        /*
         * On a 2's complement machine, negating the most negative integer 
         * results in a number that cannot be represented as a signed integer.
         * Here is what we do to obtain the number's magnitude:
         *      a. add 1 to the number
         *      b. negate it (becomes positive)
         *      c. convert it to unsigned
         *      d. add 1
         */
        if (*is_negative) {
            wide_int t = num + 1;

            magnitude = ((u_wide_int) -t) + 1;
        }
        else
            magnitude = (u_wide_int) num;
    }

    /*
     * We use a do-while loop so that we write at least 1 digit 
     */
    do {
        register u_wide_int new_magnitude = magnitude / 10;

        *--p = (char) (magnitude - new_magnitude * 10 + '0');
        magnitude = new_magnitude;
    }
    while (magnitude);

    *len = (int) (buf_end - p);
    return (p);
}

static char *conv_10_quad(widest_int num, register bool_int is_unsigned,
                     register bool_int *is_negative, char *buf_end,
                     register int *len)
{
    register char *p = buf_end;
    u_widest_int magnitude;

    /*
     * We see if we can use the faster non-quad version by checking the
     * number against the largest long value it can be. If <=, we
     * punt to the quicker version.
     */

	// ULONG_MAX and LONG_MAX are not win32 compatible  XXX

    //if ((num <= ULONG_MAX && is_unsigned) || (num <= LONG_MAX && !is_unsigned))
    //        return(conv_10( (wide_int)num, is_unsigned, is_negative,
    //           buf_end, len));

	if ((num <= 0xffffffff && is_unsigned) || (num <= 0x7fffffff && !is_unsigned))
            return(conv_10( (wide_int)num, is_unsigned, is_negative,
               buf_end, len));

    if (is_unsigned) {
        magnitude = (u_widest_int) num;
        *is_negative = FALSE;
    }
    else {
        *is_negative = (num < 0);

        /*
         * On a 2's complement machine, negating the most negative integer 
         * results in a number that cannot be represented as a signed integer.
         * Here is what we do to obtain the number's magnitude:
         *      a. add 1 to the number
         *      b. negate it (becomes positive)
         *      c. convert it to unsigned
         *      d. add 1
         */
        if (*is_negative) {
            widest_int t = num + 1;

            magnitude = ((u_widest_int) -t) + 1;
        }
        else
            magnitude = (u_widest_int) num;
    }

    /*
     * We use a do-while loop so that we write at least 1 digit 
     */
    do {
        u_widest_int new_magnitude = magnitude / 10;

        *--p = (char) (magnitude - new_magnitude * 10 + '0');
        magnitude = new_magnitude;
    }
    while (magnitude);

    *len = (int)(buf_end - p);
    return (p);
}


/*
 * Convert num to a base X number where X is a power of 2. nbits determines X.
 * For example, if nbits is 3, we do base 8 conversion
 * Return value:
 *      a pointer to a string containing the number
 *
 * The caller provides a buffer for the string: that is the buf_end argument
 * which is a pointer to the END of the buffer + 1 (i.e. if the buffer
 * is declared as buf[ 100 ], buf_end should be &buf[ 100 ])
 *
 * As with conv_10, we have a faster version which is used when
 * the number isn't quad size.
 */
static char *conv_p2(register u_wide_int num, register int nbits,
                     char format, char *buf_end, register int *len)
{
    register int mask = (1 << nbits) - 1;
    register char *p = buf_end;
    static const char low_digits[] = "0123456789abcdef";
    static const char upper_digits[] = "0123456789ABCDEF";
    register const char *digits = (format == 'X') ? upper_digits : low_digits;

    do {
        *--p = digits[num & mask];
        num >>= nbits;
    }
    while (num);

    *len = (int)(buf_end - p);
    return (p);
}

/*
 * Convert a floating point number to a string formats 'f', 'e' or 'E'.
 * The result is placed in buf, and len denotes the length of the string
 * The sign is returned in the is_negative argument (and is not placed
 * in buf).
 */
static char *conv_fp(register char format, register double num,
    int add_dp, int precision, bool_int *is_negative,
    char *buf, int *len)
{
    register char *s = buf;
    register char *p;
    int decimal_point;
    char buf1[NDIG];

    if (format == 'f')
        p = apr_fcvt(num, precision, &decimal_point, is_negative, buf1);
    else /* either e or E format */
        p = apr_ecvt(num, precision + 1, &decimal_point, is_negative, buf1);

    /*
     * Check for Infinity and NaN
     */
    if (apr_isalpha(*p)) {
        *len = (int) strlen(p);
        memcpy(buf, p, *len + 1);
        *is_negative = FALSE;
        return (buf);
    }

    if (format == 'f') {
        if (decimal_point <= 0) {
            *s++ = '0';
            if (precision > 0) {
                *s++ = '.';
                while (decimal_point++ < 0)
                    *s++ = '0';
            }
            else if (add_dp)
                *s++ = '.';
        }
        else {
            while (decimal_point-- > 0)
                *s++ = *p++;
            if (precision > 0 || add_dp)
                *s++ = '.';
        }
    }
    else {
        *s++ = *p++;
        if (precision > 0 || add_dp)
            *s++ = '.';
    }

    /*
     * copy the rest of p, the NUL is NOT copied
     */
    while (*p)
        *s++ = *p++;

    if (format != 'f') {
        char temp[EXPONENT_LENGTH];        /* for exponent conversion */
        int t_len;
        bool_int exponent_is_negative;

        *s++ = format;                /* either e or E */
        decimal_point--;
        if (decimal_point != 0) {
            p = conv_10((wide_int) decimal_point, FALSE, &exponent_is_negative,
                        &temp[EXPONENT_LENGTH], &t_len);
            *s++ = exponent_is_negative ? '-' : '+';

            /*
             * Make sure the exponent has at least 2 digits
             */
            if (t_len == 1)
                *s++ = '0';
            while (t_len--)
                *s++ = *p++;
        }
        else {
            *s++ = '+';
            *s++ = '0';
            *s++ = '0';
        }
    }

    *len = (int) (s - buf);
    return (buf);
}


/*
 * Do format conversion placing the output in buffer
 */
APR_DECLARE(int) apr_pr_vformatter(apr_vformatter_buff_t *vbuff, const char *fmt, pr_arg_p ap)
{
    register char *sp;
    register char *bep;
    register int cc = 0;
    register int i;

    register char *s = NULL;
    char *q;
    int s_len;

    register int min_width = 0;
    int precision = 0;
    enum {
        LEFT, RIGHT
    } adjust;
    char pad_char;
    char prefix_char;

    double fp_num;
    widest_int i_quad = (widest_int) 0;
    wide_int i_num = (wide_int) 0;

    char num_buf[NUM_BUF_SIZE];
    char char_buf[2];                /* for printing %% and %<unknown> */

    enum var_type_enum {
            IS_QUAD, IS_LONG, IS_SHORT, IS_INT
    };
    enum var_type_enum var_type = IS_INT;

    /*
     * Flag variables
     */
    int alternate_form;
    int print_sign;
    int print_blank;
    int adjust_precision;
    int adjust_width;
    bool_int is_negative;

	pr_arg_start(ap);

    sp = vbuff->curpos;
    bep = vbuff->endpos;

    while (*fmt) {
        if (*fmt != '%') {
            INS_CHAR(*fmt, sp, bep, cc);
        }
        else {
            /*
             * Default variable settings
             */
            int print_something = 1;
            adjust = RIGHT;
            alternate_form = print_sign = print_blank = 0;
            pad_char = ' ';
            prefix_char = NUL;

            fmt++;

            /*
             * Try to avoid checking for flags, width or precision
             */
            if (!apr_islower(*fmt)) {
                /*
                 * Recognize flags: -, #, BLANK, +
                 */
                for (;; fmt++) {
                    if (*fmt == '-')
                        adjust = LEFT;
                    else if (*fmt == '+')
                        print_sign = 1;
                    else if (*fmt == '#')
                        alternate_form = 1;
                    else if (*fmt == ' ')
                        print_blank = 1;
                    else if (*fmt == '0')
                        pad_char = '0';
                    else
                        break;
                }

                /*
                 * Check if a width was specified
                 */
                if (apr_isdigit(*fmt)) {
                    STR_TO_DEC(fmt, min_width);
                    adjust_width = 1;
                }
                else if (*fmt == '*') {
                    min_width = (int) pr_arg_int(ap); if_prerr_rtrn;
                    fmt++;
                    adjust_width = 1;
                    if (min_width < 0) {
                        adjust = LEFT;
                        min_width = -min_width;
                    }
                }
                else
                    adjust_width = 0;

                /*
                 * Check if a precision was specified
                 */
                if (*fmt == '.') {
                    adjust_precision = 1;
                    fmt++;
                    if (apr_isdigit(*fmt)) {
                        STR_TO_DEC(fmt, precision);
                    }
                    else if (*fmt == '*') {
                        precision = (int) pr_arg_int(ap); if_prerr_rtrn;
                        fmt++;
                        if (precision < 0)
                            precision = 0;
                    }
                    else
                        precision = 0;
                }
                else
                    adjust_precision = 0;
            }
            else
                adjust_precision = adjust_width = 0;

            /*
             * Modifier check
             */
#if defined(APR_INT64_T_FMT_LEN) && (APR_INT64_T_FMT_LEN == 3)
            if ((*fmt == APR_INT64_T_FMT[0]) &&
                (fmt[1] == APR_INT64_T_FMT[1])) {
#elif defined(APR_INT64_T_FMT_LEN) && (APR_INT64_T_FMT_LEN == 2)
            if (*fmt == APR_INT64_T_FMT[0]) {
#else
            if (strncmp(fmt, APR_INT64_T_FMT, 
                             sizeof(APR_INT64_T_FMT) - 2) == 0) {
#endif
                /* Need to account for trailing 'd' and null in sizeof() */
                var_type = IS_QUAD;
                fmt += (sizeof(APR_INT64_T_FMT) - 2);
            }
            else if (*fmt == 'q') {
                var_type = IS_QUAD;
                fmt++;
            }
            else if (*fmt == 'l') {
                var_type = IS_LONG;
                fmt++;
            }
            else if (*fmt == 'h') {
                var_type = IS_SHORT;
                fmt++;
            }
            else {
                var_type = IS_INT;
            }

            /*
             * Argument extraction and printing.
             * First we determine the argument type.
             * Then, we convert the argument to a string.
             * On exit from the switch, s points to the string that
             * must be printed, s_len has the length of the string
             * The precision requirements, if any, are reflected in s_len.
             *
             * NOTE: pad_char may be set to '0' because of the 0 flag.
             *   It is reset to ' ' by non-numeric formats
             */
            switch (*fmt) {
            case 'u':
                if (var_type == IS_QUAD) {
                    i_quad = pr_arg_int(ap);  if_prerr_rtrn;
                    s = conv_10_quad(i_quad, 1, &is_negative,
                            &num_buf[NUM_BUF_SIZE], &s_len);
                }
                else {
					if (var_type == IS_LONG) {
                        i_num = (wide_int) pr_arg_int(ap);  if_prerr_rtrn;
					} else if (var_type == IS_SHORT) {
                        i_num = (wide_int) (unsigned short) pr_arg_int(ap);  if_prerr_rtrn;
					} else {
                        i_num = (wide_int) pr_arg_int(ap); if_prerr_rtrn;
					}
					s = conv_10(i_num, 1, &is_negative, &num_buf[NUM_BUF_SIZE], &s_len);
                }
                FIX_PRECISION(adjust_precision, precision, s, s_len);
                break;

            case 'd':
            case 'i':
                if (var_type == IS_QUAD) {
                    i_quad = pr_arg_int(ap); if_prerr_rtrn;
                    s = conv_10_quad(i_quad, 0, &is_negative,
                            &num_buf[NUM_BUF_SIZE], &s_len);
                }
                else {
					if (var_type == IS_LONG) {
                        i_num = (wide_int) pr_arg_int(ap); if_prerr_rtrn;
					} else if (var_type == IS_SHORT) {
                        i_num = (wide_int) (short) pr_arg_int(ap); if_prerr_rtrn;
					} else {
                        i_num = (wide_int) pr_arg_int(ap); if_prerr_rtrn;
					}
					s = conv_10(i_num, 0, &is_negative,
                            &num_buf[NUM_BUF_SIZE], &s_len);
                }
                FIX_PRECISION(adjust_precision, precision, s, s_len);

                if (is_negative)
                    prefix_char = '-';
                else if (print_sign)
                    prefix_char = '+';
                else if (print_blank)
                    prefix_char = ' ';
                break;

            case 'x':
            case 'X':
                if (var_type == IS_QUAD) {
                    i_quad = pr_arg_int(ap); if_prerr_rtrn;
                    s = conv_10_quad(i_quad, 0, &is_negative,
                            &num_buf[NUM_BUF_SIZE], &s_len);
                }
                else {
                    if (var_type == IS_LONG)
                        i_num = (wide_int) pr_arg_int(ap);  if_prerr_rtrn;
                    else if (var_type == IS_SHORT)
                        i_num = (wide_int) (short) pr_arg_int(ap); if_prerr_rtrn;
					else
                        i_num = (wide_int) pr_arg_int(ap); if_prerr_rtrn;
                    s = conv_p2(i_num, 4, *fmt,
                            &num_buf[NUM_BUF_SIZE], &s_len);
                }
                FIX_PRECISION(adjust_precision, precision, s, s_len);
                if (alternate_form && i_num != 0) {
                    *--s = *fmt;        /* 'x' or 'X' */
                    *--s = '0';
                    s_len += 2;
                }
                break;

            case 's':
                s = pr_arg_string(ap);  
				if_prerr_rtrn;
                if (s != NULL) {
                    if (!adjust_precision) {
                        s_len = (int) strlen(s);
                    }
                    else {
                        /* From the C library standard in section 7.9.6.1:
                         * ...if the precision is specified, no more then
                         * that many characters are written.  If the
                         * precision is not specified or is greater
                         * than the size of the array, the array shall
                         * contain a null character.
                         *
                         * My reading is if precision is specified and
                         * is less then or equal to the size of the
                         * array, no null character is required.  So
                         * we can't do a strlen.
                         *
                         * This figures out the length of the string
                         * up to the precision.  Once it's long enough
                         * for the specified precision, we don't care
                         * anymore.
                         *
                         * NOTE: you must do the length comparison
                         * before the check for the null character.
                         * Otherwise, you'll check one beyond the
                         * last valid character.
                         */
                        const char *walk;

                        for (walk = s, s_len = 0;
                             (s_len < precision) && (*walk != '\0');
                             ++walk, ++s_len);
                    }
                }
                else {
                    s = S_NULL;
                    s_len = S_NULL_LEN;
                }
                pad_char = ' ';
                break;

            case 'f':
            case 'e':
            case 'E':
                fp_num = pr_arg_float(ap);  if_prerr_rtrn;
                /*
                 * We use &num_buf[ 1 ], so that we have room for the sign
                 */
                s = NULL;
#ifdef HAVE_ISNAN
                if (isnan(fp_num)) {
                    s = "nan";
                    s_len = 3;
                }
#endif
#ifdef HAVE_ISINF
                if (!s && isinf(fp_num)) {
                    s = "inf";
                    s_len = 3;
                }
#endif
                if (!s) {
                    s = conv_fp(*fmt, fp_num, alternate_form,
                            (adjust_precision == 0) ? FLOAT_DIGITS : precision,
                                &is_negative, &num_buf[1], &s_len);
                    if (is_negative)
                        prefix_char = '-';
                    else if (print_sign)
                        prefix_char = '+';
                    else if (print_blank)
                        prefix_char = ' ';
                }
                break;

            case 'g':
            case 'G':
                if (adjust_precision == 0)
                    precision = FLOAT_DIGITS;
                else if (precision == 0)
                    precision = 1;
                /*
                 * * We use &num_buf[ 1 ], so that we have room for the sign
                 */
                s = apr_gcvt(pr_arg_float(ap), precision, &num_buf[1], alternate_form);
				if_prerr_rtrn;
                if (*s == '-')
                    prefix_char = *s++;
                else if (print_sign)
                    prefix_char = '+';
                else if (print_blank)
                    prefix_char = ' ';

                s_len = (int) strlen(s);

                if (alternate_form && (q = strchr(s, '.')) == NULL) {
                    s[s_len++] = '.';
                    s[s_len] = '\0'; /* delimit for following strchr() */
                }
                if (*fmt == 'G' && (q = strchr(s, 'e')) != NULL)
                    *q = 'E';
                break;

            case 'c':
                char_buf[0] = (char) (pr_arg_string(ap)[0]);
				if_prerr_rtrn;
                s = &char_buf[0];
                s_len = 1;
                pad_char = ' ';
                break;

            case '%':
                char_buf[0] = '%';
                s = &char_buf[0];
                s_len = 1;
                pad_char = ' ';
                break;

            case NUL:
                /*
                 * The last character of the format string was %.
                 * We ignore it.
                 */
                continue;

                /*
                 * The default case is for unrecognized %'s.
                 * We print %<char> to help the user identify what
                 * option is not understood.
                 * This is also useful in case the user wants to pass
                 * the output of format_converter to another function
                 * that understands some other %<char> (like syslog).
                 * Note that we can't point s inside fmt because the
                 * unknown <char> could be preceded by width etc.
                 */
            default:
                char_buf[0] = '%';
                char_buf[1] = *fmt;
                s = char_buf;
                s_len = 2;
                pad_char = ' ';
                break;
            }

            if (prefix_char != NUL && s != S_NULL && s != char_buf) {
                *--s = prefix_char;
                s_len++;
            }

            if (adjust_width && adjust == RIGHT && min_width > s_len) {
                if (pad_char == '0' && prefix_char != NUL) {
                    INS_CHAR(*s, sp, bep, cc);
                    s++;
                    s_len--;
                    min_width--;
                }
                PAD(min_width, s_len, pad_char);
            }

            /*
             * Print the string s. 
             */
            if (print_something == 1) {
                for (i = s_len; i != 0; i--) {
                      INS_CHAR(*s, sp, bep, cc);
                    s++;
                }
            }

            if (adjust_width && adjust == LEFT && min_width > s_len)
                PAD(min_width, s_len, pad_char);
        }
        fmt++;
    }
    vbuff->curpos = sp;

    return cc;
}


APR_DECLARE(int) apr_pr_vsnprintf(char *buf, apr_size_t len, const char *format, pr_arg_p ap) {
    int cc;
    apr_vformatter_buff_t vbuff;

    if (len == 0) {
        /* See above note */
        vbuff.curpos = NULL;
        vbuff.endpos = NULL;
    } else {
        /* save one byte for nul terminator */
        vbuff.curpos = buf;
        vbuff.endpos = buf + len - 1;
    }
    cc = apr_pr_vformatter(&vbuff, format, ap);
	if (cc == -99) return -99;
    if (len != 0) {
        *vbuff.curpos = '\0';
    }
    return (cc == -1) ? (int)len : cc;
}
