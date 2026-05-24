/*
 * stdint.h - C99 fixed-width integer shim for the Xbox XDK (RXDK)
 *
 * The XDK does not ship stdint.h. The Xbox CPU is a Pentium III (x86),
 * so sizes are well-defined and we can hard-code them safely.
 */
#ifndef _STDINT_H
#define _STDINT_H

/* ---- Exact-width signed ---- */
typedef signed char        int8_t;
typedef short              int16_t;
typedef int                int32_t;
typedef __int64            int64_t;

/* ---- Exact-width unsigned ---- */
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned __int64   uint64_t;

/* ---- Least-width ---- */
typedef int8_t             int_least8_t;
typedef int16_t            int_least16_t;
typedef int32_t            int_least32_t;
typedef int64_t            int_least64_t;

typedef uint8_t            uint_least8_t;
typedef uint16_t           uint_least16_t;
typedef uint32_t           uint_least32_t;
typedef uint64_t           uint_least64_t;

/* ---- Fast ---- */
typedef int8_t             int_fast8_t;
typedef int32_t            int_fast16_t;
typedef int32_t            int_fast32_t;
typedef int64_t            int_fast64_t;

typedef uint8_t            uint_fast8_t;
typedef uint32_t           uint_fast16_t;
typedef uint32_t           uint_fast32_t;
typedef uint64_t           uint_fast64_t;

/* ---- Pointer-sized ---- */
typedef int                intptr_t;
typedef unsigned int       uintptr_t;

/* ---- Max-width ---- */
typedef int64_t            intmax_t;
typedef uint64_t           uintmax_t;

/* ---- Limits ---- */
#define INT8_MIN    (-128)
#define INT8_MAX    (127)
#define UINT8_MAX   (255U)

#define INT16_MIN   (-32768)
#define INT16_MAX   (32767)
#define UINT16_MAX  (65535U)

#define INT32_MIN   (-2147483647 - 1)
#define INT32_MAX   (2147483647)
#define UINT32_MAX  (4294967295U)

#define INT64_MIN   (-9223372036854775807i64 - 1)
#define INT64_MAX   (9223372036854775807i64)
#define UINT64_MAX  (18446744073709551615ui64)

#define INTPTR_MIN  INT32_MIN
#define INTPTR_MAX  INT32_MAX
#define UINTPTR_MAX UINT32_MAX

#define INTMAX_MIN  INT64_MIN
#define INTMAX_MAX  INT64_MAX
#define UINTMAX_MAX UINT64_MAX

/* ---- Format macros (inttypes.h subset) ---- */
#define PRId32  "d"
#define PRIu32  "u"
#define PRId64  "I64d"
#define PRIu64  "I64u"

#endif /* _STDINT_H */
