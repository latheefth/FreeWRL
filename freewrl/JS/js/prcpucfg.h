#ifndef nspr_cpucfg___
#define nspr_cpucfg___

#if defined(XP_MAC) || defined(XP_OSX)
#undef  IS_LITTLE_ENDIAN
#define IS_BIG_ENDIAN 1

#define PR_BYTES_PER_BYTE   1L
#define PR_BYTES_PER_SHORT  2L
#define PR_BYTES_PER_INT    4L
#define PR_BYTES_PER_INT64  8L
#define PR_BYTES_PER_LONG   4L
#define PR_BYTES_PER_FLOAT  4L
#define PR_BYTES_PER_DOUBLE 8L
#define PR_BYTES_PER_WORD   4L
#define PR_BYTES_PER_DWORD  8L

#define PR_BITS_PER_BYTE    8L
#define PR_BITS_PER_SHORT   16L
#define PR_BITS_PER_INT     32L
#define PR_BITS_PER_INT64   64L
#define PR_BITS_PER_LONG    32L
#define PR_BITS_PER_FLOAT   32L
#define PR_BITS_PER_DOUBLE  64L
#define PR_BITS_PER_WORD    32L

#define PR_BITS_PER_BYTE_LOG2   3L
#define PR_BITS_PER_SHORT_LOG2  4L
#define PR_BITS_PER_INT_LOG2    5L
#define PR_BITS_PER_INT64_LOG2  6L
#define PR_BITS_PER_LONG_LOG2   5L
#define PR_BITS_PER_FLOAT_LOG2  5L
#define PR_BITS_PER_DOUBLE_LOG2 6L
#define PR_BITS_PER_WORD_LOG2   5L

#define PR_ALIGN_OF_SHORT   2L
#define PR_ALIGN_OF_INT     4L
#define PR_ALIGN_OF_LONG    4L
#define PR_ALIGN_OF_INT64   2L
#define PR_ALIGN_OF_FLOAT   4L
#define PR_ALIGN_OF_DOUBLE  4L
#define PR_ALIGN_OF_POINTER 4L
#define PR_ALIGN_OF_WORD    4L

#define PR_BYTES_PER_WORD_LOG2   2L
#define PR_BYTES_PER_DWORD_LOG2  3L
#define PR_WORDS_PER_DWORD_LOG2  1L
#endif /* XP_MAC */

#ifdef _WIN32
#define IS_LITTLE_ENDIAN 1
#undef  IS_BIG_ENDIAN

#define PR_BYTES_PER_BYTE   1L
#define PR_BYTES_PER_SHORT  2L
#define PR_BYTES_PER_INT    4L
#define PR_BYTES_PER_INT64  8L
#define PR_BYTES_PER_LONG   4L
#define PR_BYTES_PER_FLOAT  4L
#define PR_BYTES_PER_DOUBLE 8L
#define PR_BYTES_PER_WORD   4L
#define PR_BYTES_PER_DWORD  8L

#define PR_BITS_PER_BYTE    8L
#define PR_BITS_PER_SHORT   16L
#define PR_BITS_PER_INT     32L
#define PR_BITS_PER_INT64   64L
#define PR_BITS_PER_LONG    32L
#define PR_BITS_PER_FLOAT   32L
#define PR_BITS_PER_DOUBLE  64L
#define PR_BITS_PER_WORD    32L

#define PR_BITS_PER_BYTE_LOG2   3L
#define PR_BITS_PER_SHORT_LOG2  4L
#define PR_BITS_PER_INT_LOG2    5L
#define PR_BITS_PER_INT64_LOG2  6L
#define PR_BITS_PER_LONG_LOG2   5L
#define PR_BITS_PER_FLOAT_LOG2  5L
#define PR_BITS_PER_DOUBLE_LOG2 6L
#define PR_BITS_PER_WORD_LOG2   5L

#define PR_ALIGN_OF_SHORT   2L
#define PR_ALIGN_OF_INT     4L
#define PR_ALIGN_OF_LONG    4L
#define PR_ALIGN_OF_INT64   8L
#define PR_ALIGN_OF_FLOAT   4L
#define PR_ALIGN_OF_DOUBLE  4L
#define PR_ALIGN_OF_POINTER 4L
#define PR_ALIGN_OF_WORD    4L

#define PR_BYTES_PER_WORD_LOG2   2L
#define PR_BYTES_PER_DWORD_LOG2  3L
#define PR_WORDS_PER_DWORD_LOG2  1L
#endif /* _WIN32 */

#if defined(_WINDOWS) && !defined(_WIN32) /* WIN16 */
#define IS_LITTLE_ENDIAN 1
#undef  IS_BIG_ENDIAN

#define PR_BYTES_PER_BYTE   1L
#define PR_BYTES_PER_SHORT  2L
#define PR_BYTES_PER_INT    2L
#define PR_BYTES_PER_INT64  8L
#define PR_BYTES_PER_LONG   4L
#define PR_BYTES_PER_FLOAT  4L
#define PR_BYTES_PER_DOUBLE 8L
#define PR_BYTES_PER_WORD   4L
#define PR_BYTES_PER_DWORD  8L

#define PR_BITS_PER_BYTE    8L
#define PR_BITS_PER_SHORT   16L
#define PR_BITS_PER_INT     16L
#define PR_BITS_PER_INT64   64L
#define PR_BITS_PER_LONG    32L
#define PR_BITS_PER_FLOAT   32L
#define PR_BITS_PER_DOUBLE  64L
#define PR_BITS_PER_WORD    32L

#define PR_BITS_PER_BYTE_LOG2   3L
#define PR_BITS_PER_SHORT_LOG2  4L
#define PR_BITS_PER_INT_LOG2    4L
#define PR_BITS_PER_INT64_LOG2  6L
#define PR_BITS_PER_LONG_LOG2   5L
#define PR_BITS_PER_FLOAT_LOG2  5L
#define PR_BITS_PER_DOUBLE_LOG2 6L
#define PR_BITS_PER_WORD_LOG2   5L

#define PR_ALIGN_OF_SHORT   2L
#define PR_ALIGN_OF_INT     2L
#define PR_ALIGN_OF_LONG    2L
#define PR_ALIGN_OF_INT64   2L
#define PR_ALIGN_OF_FLOAT   2L
#define PR_ALIGN_OF_DOUBLE  2L
#define PR_ALIGN_OF_POINTER 2L
#define PR_ALIGN_OF_WORD    2L

#define PR_BYTES_PER_WORD_LOG2   2L
#define PR_BYTES_PER_DWORD_LOG2  3L
#define PR_WORDS_PER_DWORD_LOG2  1L
#endif /* defined(_WINDOWS) && !defined(_WIN32) */

#if defined(XP_UNIX) && !defined(XP_OSX)

#if defined(AIXV3)
#undef  IS_LITTLE_ENDIAN
#define IS_BIG_ENDIAN 1

#define PR_BYTES_PER_BYTE   1L
#define PR_BYTES_PER_SHORT  2L
#define PR_BYTES_PER_INT    4L
#define PR_BYTES_PER_INT64  8L
#define PR_BYTES_PER_LONG   4L
#define PR_BYTES_PER_FLOAT  4L
#define PR_BYTES_PER_DOUBLE 8L
#define PR_BYTES_PER_WORD   4L
#define PR_BYTES_PER_DWORD  8L

#define PR_BITS_PER_BYTE    8L
#define PR_BITS_PER_SHORT   16L
#define PR_BITS_PER_INT     32L
#define PR_BITS_PER_INT64   64L
#define PR_BITS_PER_LONG    32L
#define PR_BITS_PER_FLOAT   32L
#define PR_BITS_PER_DOUBLE  64L
#define PR_BITS_PER_WORD    32L

#define PR_BITS_PER_BYTE_LOG2   3L
#define PR_BITS_PER_SHORT_LOG2  4L
#define PR_BITS_PER_INT_LOG2    5L
#define PR_BITS_PER_INT64_LOG2  6L
#define PR_BITS_PER_LONG_LOG2   5L
#define PR_BITS_PER_FLOAT_LOG2  5L
#define PR_BITS_PER_DOUBLE_LOG2 6L
#define PR_BITS_PER_WORD_LOG2   5L

#define PR_ALIGN_OF_SHORT   2L
#define PR_ALIGN_OF_INT     4L
#define PR_ALIGN_OF_LONG    4L
#define PR_ALIGN_OF_INT64   8L
#define PR_ALIGN_OF_FLOAT   4L
#define PR_ALIGN_OF_DOUBLE  4L
#define PR_ALIGN_OF_POINTER 4L
#define PR_ALIGN_OF_WORD    4L

#define PR_BYTES_PER_WORD_LOG2   2L
#define PR_BYTES_PER_DWORD_LOG2  3L
#define PR_WORDS_PER_DWORD_LOG2  1L


#elif defined(BSDI)
#define IS_LITTLE_ENDIAN 1
#undef  IS_BIG_ENDIAN

#define PR_BYTES_PER_BYTE   1L
#define PR_BYTES_PER_SHORT  2L
#define PR_BYTES_PER_INT    4L
#define PR_BYTES_PER_INT64  8L
#define PR_BYTES_PER_LONG   4L
#define PR_BYTES_PER_FLOAT  4L
#define PR_BYTES_PER_DOUBLE 8L
#define PR_BYTES_PER_WORD   4L
#define PR_BYTES_PER_DWORD  8L

#define PR_BITS_PER_BYTE    8L
#define PR_BITS_PER_SHORT   16L
#define PR_BITS_PER_INT     32L
#define PR_BITS_PER_INT64   64L
#define PR_BITS_PER_LONG    32L
#define PR_BITS_PER_FLOAT   32L
#define PR_BITS_PER_DOUBLE  64L
#define PR_BITS_PER_WORD    32L

#define PR_BITS_PER_BYTE_LOG2   3L
#define PR_BITS_PER_SHORT_LOG2  4L
#define PR_BITS_PER_INT_LOG2    5L
#define PR_BITS_PER_INT64_LOG2  6L
#define PR_BITS_PER_LONG_LOG2   5L
#define PR_BITS_PER_FLOAT_LOG2  5L
#define PR_BITS_PER_DOUBLE_LOG2 6L
#define PR_BITS_PER_WORD_LOG2   5L

#define PR_ALIGN_OF_SHORT   2L
#define PR_ALIGN_OF_INT     4L
#define PR_ALIGN_OF_LONG    4L
#define PR_ALIGN_OF_INT64   4L
#define PR_ALIGN_OF_FLOAT   4L
#define PR_ALIGN_OF_DOUBLE  4L
#define PR_ALIGN_OF_POINTER 4L
#define PR_ALIGN_OF_WORD    4L

#define PR_BYTES_PER_WORD_LOG2   2L
#define PR_BYTES_PER_DWORD_LOG2  3L
#define PR_WORDS_PER_DWORD_LOG2  1L


#elif defined(HPUX)
#undef  IS_LITTLE_ENDIAN
#define IS_BIG_ENDIAN 1

#define PR_BYTES_PER_BYTE   1L
#define PR_BYTES_PER_SHORT  2L
#define PR_BYTES_PER_INT    4L
#define PR_BYTES_PER_INT64  8L
#define PR_BYTES_PER_LONG   4L
#define PR_BYTES_PER_FLOAT  4L
#define PR_BYTES_PER_DOUBLE 8L
#define PR_BYTES_PER_WORD   4L
#define PR_BYTES_PER_DWORD  8L

#define PR_BITS_PER_BYTE    8L
#define PR_BITS_PER_SHORT   16L
#define PR_BITS_PER_INT     32L
#define PR_BITS_PER_INT64   64L
#define PR_BITS_PER_LONG    32L
#define PR_BITS_PER_FLOAT   32L
#define PR_BITS_PER_DOUBLE  64L
#define PR_BITS_PER_WORD    32L

#define PR_BITS_PER_BYTE_LOG2   3L
#define PR_BITS_PER_SHORT_LOG2  4L
#define PR_BITS_PER_INT_LOG2    5L
#define PR_BITS_PER_INT64_LOG2  6L
#define PR_BITS_PER_LONG_LOG2   5L
#define PR_BITS_PER_FLOAT_LOG2  5L
#define PR_BITS_PER_DOUBLE_LOG2 6L
#define PR_BITS_PER_WORD_LOG2   5L

#define PR_ALIGN_OF_SHORT   2L
#define PR_ALIGN_OF_INT     4L
#define PR_ALIGN_OF_LONG    4L
#define PR_ALIGN_OF_INT64   4L
#define PR_ALIGN_OF_FLOAT   4L
#define PR_ALIGN_OF_DOUBLE  8L
#define PR_ALIGN_OF_POINTER 4L
#define PR_ALIGN_OF_WORD    4L

#define PR_BYTES_PER_WORD_LOG2   2L
#define PR_BYTES_PER_DWORD_LOG2  3L
#define PR_WORDS_PER_DWORD_LOG2  1L


#elif defined(IRIX)
#undef  IS_LITTLE_ENDIAN
#define IS_BIG_ENDIAN 1

#define PR_BYTES_PER_BYTE   1L
#define PR_BYTES_PER_SHORT  2L
#define PR_BYTES_PER_INT    4L
#define PR_BYTES_PER_INT64  8L
#define PR_BYTES_PER_LONG   4L
#define PR_BYTES_PER_FLOAT  4L
#define PR_BYTES_PER_DOUBLE 8L
#define PR_BYTES_PER_WORD   4L
#define PR_BYTES_PER_DWORD  8L

#define PR_BITS_PER_BYTE    8L
#define PR_BITS_PER_SHORT   16L
#define PR_BITS_PER_INT     32L
#define PR_BITS_PER_INT64   64L
#define PR_BITS_PER_LONG    32L
#define PR_BITS_PER_FLOAT   32L
#define PR_BITS_PER_DOUBLE  64L
#define PR_BITS_PER_WORD    32L

#define PR_BITS_PER_BYTE_LOG2   3L
#define PR_BITS_PER_SHORT_LOG2  4L
#define PR_BITS_PER_INT_LOG2    5L
#define PR_BITS_PER_INT64_LOG2  6L
#define PR_BITS_PER_LONG_LOG2   5L
#define PR_BITS_PER_FLOAT_LOG2  5L
#define PR_BITS_PER_DOUBLE_LOG2 6L
#define PR_BITS_PER_WORD_LOG2   5L

#define PR_ALIGN_OF_SHORT   2L
#define PR_ALIGN_OF_INT     4L
#define PR_ALIGN_OF_LONG    4L
#define PR_ALIGN_OF_INT64   8L
#define PR_ALIGN_OF_FLOAT   4L
#define PR_ALIGN_OF_DOUBLE  8L
#define PR_ALIGN_OF_POINTER 4L
#define PR_ALIGN_OF_WORD    4L

#define PR_BYTES_PER_WORD_LOG2   2L
#define PR_BYTES_PER_DWORD_LOG2  3L
#define PR_WORDS_PER_DWORD_LOG2  1L


#elif defined(LINUX)

#ifdef __powerpc__
#undef  IS_LITTLE_ENDIAN
#define IS_BIG_ENDIAN    1
#elif  __i386__
#define IS_LITTLE_ENDIAN 1
#undef  IS_BIG_ENDIAN
#else
#error "linux cpu architecture not supported by prcpucfg.h"
#endif

#define PR_BYTES_PER_BYTE   1L
#define PR_BYTES_PER_SHORT  2L
#define PR_BYTES_PER_INT    4L
#define PR_BYTES_PER_INT64  8L
#define PR_BYTES_PER_LONG   4L
#define PR_BYTES_PER_FLOAT  4L
#define PR_BYTES_PER_DOUBLE 8L
#define PR_BYTES_PER_WORD   4L
#define PR_BYTES_PER_DWORD  8L

#define PR_BITS_PER_BYTE    8L
#define PR_BITS_PER_SHORT   16L
#define PR_BITS_PER_INT     32L
#define PR_BITS_PER_INT64   64L
#define PR_BITS_PER_LONG    32L
#define PR_BITS_PER_FLOAT   32L
#define PR_BITS_PER_DOUBLE  64L
#define PR_BITS_PER_WORD    32L

#define PR_BITS_PER_BYTE_LOG2   3L
#define PR_BITS_PER_SHORT_LOG2  4L
#define PR_BITS_PER_INT_LOG2    5L
#define PR_BITS_PER_INT64_LOG2  6L
#define PR_BITS_PER_LONG_LOG2   5L
#define PR_BITS_PER_FLOAT_LOG2  5L
#define PR_BITS_PER_DOUBLE_LOG2 6L
#define PR_BITS_PER_WORD_LOG2   5L

#define PR_ALIGN_OF_SHORT   2L
#define PR_ALIGN_OF_INT     4L
#define PR_ALIGN_OF_LONG    4L
#define PR_ALIGN_OF_INT64   4L
#define PR_ALIGN_OF_FLOAT   4L
#define PR_ALIGN_OF_DOUBLE  4L
#define PR_ALIGN_OF_POINTER 4L
#define PR_ALIGN_OF_WORD    4L

#define PR_BYTES_PER_WORD_LOG2   2L
#define PR_BYTES_PER_DWORD_LOG2  3L
#define PR_WORDS_PER_DWORD_LOG2  1L


#elif defined(OSF1)
#define IS_LITTLE_ENDIAN 1
#undef  IS_BIG_ENDIAN

#define PR_BYTES_PER_BYTE   1L
#define PR_BYTES_PER_SHORT  2L
#define PR_BYTES_PER_INT    4L
#define PR_BYTES_PER_INT64  8L
#define PR_BYTES_PER_LONG   8L
#define PR_BYTES_PER_FLOAT  4L
#define PR_BYTES_PER_DOUBLE 8L
#define PR_BYTES_PER_WORD   8L
#define PR_BYTES_PER_DWORD  8L

#define PR_BITS_PER_BYTE    8L
#define PR_BITS_PER_SHORT   16L
#define PR_BITS_PER_INT     32L
#define PR_BITS_PER_INT64   64L
#define PR_BITS_PER_LONG    64L
#define PR_BITS_PER_FLOAT   32L
#define PR_BITS_PER_DOUBLE  64L
#define PR_BITS_PER_WORD    64L

#define PR_BITS_PER_BYTE_LOG2   3L
#define PR_BITS_PER_SHORT_LOG2  4L
#define PR_BITS_PER_INT_LOG2    5L
#define PR_BITS_PER_INT64_LOG2  6L
#define PR_BITS_PER_LONG_LOG2   6L
#define PR_BITS_PER_FLOAT_LOG2  5L
#define PR_BITS_PER_DOUBLE_LOG2 6L
#define PR_BITS_PER_WORD_LOG2   6L

#define PR_ALIGN_OF_SHORT   2L
#define PR_ALIGN_OF_INT     4L
#define PR_ALIGN_OF_LONG    8L
#define PR_ALIGN_OF_INT64   8L
#define PR_ALIGN_OF_FLOAT   4L
#define PR_ALIGN_OF_DOUBLE  8L
#define PR_ALIGN_OF_POINTER 8L
#define PR_ALIGN_OF_WORD    8L

#define PR_BYTES_PER_WORD_LOG2   3L
#define PR_BYTES_PER_DWORD_LOG2  3L
#define PR_WORDS_PER_DWORD_LOG2  0L


#elif defined(SOLARIS)

#ifdef i386
/* PC-based */
#undef  IS_BIG_ENDIAN
#define IS_LITTLE_ENDIAN 1
#else
/* Sparc-based */
#undef  IS_LITTLE_ENDIAN
#define IS_BIG_ENDIAN 1
#endif

#define PR_BYTES_PER_BYTE   1L
#define PR_BYTES_PER_SHORT  2L
#define PR_BYTES_PER_INT    4L
#define PR_BYTES_PER_INT64  8L
#define PR_BYTES_PER_LONG   4L
#define PR_BYTES_PER_FLOAT  4L
#define PR_BYTES_PER_DOUBLE 8L
#define PR_BYTES_PER_WORD   4L
#define PR_BYTES_PER_DWORD  8L

#define PR_BITS_PER_BYTE    8L
#define PR_BITS_PER_SHORT   16L
#define PR_BITS_PER_INT     32L
#define PR_BITS_PER_INT64   64L
#define PR_BITS_PER_LONG    32L
#define PR_BITS_PER_FLOAT   32L
#define PR_BITS_PER_DOUBLE  64L
#define PR_BITS_PER_WORD    32L

#define PR_BITS_PER_BYTE_LOG2   3L
#define PR_BITS_PER_SHORT_LOG2  4L
#define PR_BITS_PER_INT_LOG2    5L
#define PR_BITS_PER_INT64_LOG2  6L
#define PR_BITS_PER_LONG_LOG2   5L
#define PR_BITS_PER_FLOAT_LOG2  5L
#define PR_BITS_PER_DOUBLE_LOG2 6L
#define PR_BITS_PER_WORD_LOG2   5L

#define PR_ALIGN_OF_SHORT   2L
#define PR_ALIGN_OF_INT     4L
#define PR_ALIGN_OF_LONG    4L
#define PR_ALIGN_OF_INT64   8L
#define PR_ALIGN_OF_FLOAT   4L
#define PR_ALIGN_OF_DOUBLE  8L
#define PR_ALIGN_OF_POINTER 4L
#define PR_ALIGN_OF_WORD    4L

#ifdef i386
#undef PR_ALIGN_OF_INT64
#undef PR_ALIGN_OF_DOUBLE
#define PR_ALIGN_OF_INT64   4L
#define PR_ALIGN_OF_DOUBLE  4L
#endif

#define PR_BYTES_PER_WORD_LOG2   2L
#define PR_BYTES_PER_DWORD_LOG2  3L
#define PR_WORDS_PER_DWORD_LOG2  1L


#elif defined(SUNOS4)
#undef  IS_LITTLE_ENDIAN
#define IS_BIG_ENDIAN 1

#define PR_BYTES_PER_BYTE   1L
#define PR_BYTES_PER_SHORT  2L
#define PR_BYTES_PER_INT    4L
#define PR_BYTES_PER_INT64  8L
#define PR_BYTES_PER_LONG   4L
#define PR_BYTES_PER_FLOAT  4L
#define PR_BYTES_PER_DOUBLE 8L
#define PR_BYTES_PER_WORD   4L
#define PR_BYTES_PER_DWORD  8L

#define PR_BITS_PER_BYTE    8L
#define PR_BITS_PER_SHORT   16L
#define PR_BITS_PER_INT     32L
#define PR_BITS_PER_INT64   64L
#define PR_BITS_PER_LONG    32L
#define PR_BITS_PER_FLOAT   32L
#define PR_BITS_PER_DOUBLE  64L
#define PR_BITS_PER_WORD    32L

#define PR_BITS_PER_BYTE_LOG2   3L
#define PR_BITS_PER_SHORT_LOG2  4L
#define PR_BITS_PER_INT_LOG2    5L
#define PR_BITS_PER_INT64_LOG2  6L
#define PR_BITS_PER_LONG_LOG2   5L
#define PR_BITS_PER_FLOAT_LOG2  5L
#define PR_BITS_PER_DOUBLE_LOG2 6L
#define PR_BITS_PER_WORD_LOG2   5L

#define PR_ALIGN_OF_SHORT   2L
#define PR_ALIGN_OF_INT     4L
#define PR_ALIGN_OF_LONG    4L
#define PR_ALIGN_OF_INT64   8L
#define PR_ALIGN_OF_FLOAT   4L
#define PR_ALIGN_OF_DOUBLE  8L
#define PR_ALIGN_OF_POINTER 4L
#define PR_ALIGN_OF_WORD    4L

#define PR_BYTES_PER_WORD_LOG2   2L
#define PR_BYTES_PER_DWORD_LOG2  3L
#define PR_WORDS_PER_DWORD_LOG2  1L


#elif defined(SNI)
#undef  IS_LITTLE_ENDIAN
#define IS_BIG_ENDIAN 1

#define PR_BYTES_PER_BYTE   1L
#define PR_BYTES_PER_SHORT  2L
#define PR_BYTES_PER_INT    4L
#define PR_BYTES_PER_INT64  8L
#define PR_BYTES_PER_LONG   4L
#define PR_BYTES_PER_FLOAT  4L
#define PR_BYTES_PER_DOUBLE 8L
#define PR_BYTES_PER_WORD   4L
#define PR_BYTES_PER_DWORD  8L

#define PR_BITS_PER_BYTE    8L
#define PR_BITS_PER_SHORT   16L
#define PR_BITS_PER_INT     32L
#define PR_BITS_PER_INT64   64L
#define PR_BITS_PER_LONG    32L
#define PR_BITS_PER_FLOAT   32L
#define PR_BITS_PER_DOUBLE  64L
#define PR_BITS_PER_WORD    32L

#define PR_BITS_PER_BYTE_LOG2   3L
#define PR_BITS_PER_SHORT_LOG2  4L
#define PR_BITS_PER_INT_LOG2    5L
#define PR_BITS_PER_INT64_LOG2  6L
#define PR_BITS_PER_LONG_LOG2   5L
#define PR_BITS_PER_FLOAT_LOG2  5L
#define PR_BITS_PER_DOUBLE_LOG2 6L
#define PR_BITS_PER_WORD_LOG2   5L

#define PR_ALIGN_OF_SHORT   2L
#define PR_ALIGN_OF_INT     4L
#define PR_ALIGN_OF_LONG    4L
#define PR_ALIGN_OF_INT64   8L
#define PR_ALIGN_OF_FLOAT   4L
#define PR_ALIGN_OF_DOUBLE  8L
#define PR_ALIGN_OF_POINTER 4L
#define PR_ALIGN_OF_WORD    4L

#define PR_BYTES_PER_WORD_LOG2   2L
#define PR_BYTES_PER_DWORD_LOG2  3L
#define PR_WORDS_PER_DWORD_LOG2  1L


#elif defined(SONY)
/* Don't have it */

#elif defined(NECSVR4)
/* Don't have it */

#elif defined(SCO)
#define IS_LITTLE_ENDIAN 1
#undef  IS_BIG_ENDIAN

#define PR_BYTES_PER_BYTE   1L
#define PR_BYTES_PER_SHORT  2L
#define PR_BYTES_PER_INT    4L
#define PR_BYTES_PER_INT64  8L
#define PR_BYTES_PER_LONG   4L
#define PR_BYTES_PER_FLOAT  4L
#define PR_BYTES_PER_DOUBLE 8L
#define PR_BYTES_PER_WORD   4L
#define PR_BYTES_PER_DWORD  8L

#define PR_BITS_PER_BYTE    8L
#define PR_BITS_PER_SHORT   16L
#define PR_BITS_PER_INT     32L
#define PR_BITS_PER_INT64   64L
#define PR_BITS_PER_LONG    32L
#define PR_BITS_PER_FLOAT   32L
#define PR_BITS_PER_DOUBLE  64L
#define PR_BITS_PER_WORD    32L

#define PR_BITS_PER_BYTE_LOG2   3L
#define PR_BITS_PER_SHORT_LOG2  4L
#define PR_BITS_PER_INT_LOG2    5L
#define PR_BITS_PER_INT64_LOG2  6L
#define PR_BITS_PER_LONG_LOG2   5L
#define PR_BITS_PER_FLOAT_LOG2  5L
#define PR_BITS_PER_DOUBLE_LOG2 6L
#define PR_BITS_PER_WORD_LOG2   5L

#define PR_ALIGN_OF_SHORT   2L
#define PR_ALIGN_OF_INT     4L
#define PR_ALIGN_OF_LONG    4L
#define PR_ALIGN_OF_INT64   4L
#define PR_ALIGN_OF_FLOAT   4L
#define PR_ALIGN_OF_DOUBLE  4L
#define PR_ALIGN_OF_POINTER 4L
#define PR_ALIGN_OF_WORD    4L

#define PR_BYTES_PER_WORD_LOG2   2L
#define PR_BYTES_PER_DWORD_LOG2  3L
#define PR_WORDS_PER_DWORD_LOG2  1L


#elif defined(UNIXWARE)
#define IS_LITTLE_ENDIAN 1
#undef  IS_BIG_ENDIAN

#define PR_BYTES_PER_BYTE   1L
#define PR_BYTES_PER_SHORT  2L
#define PR_BYTES_PER_INT    4L
#define PR_BYTES_PER_INT64  8L
#define PR_BYTES_PER_LONG   4L
#define PR_BYTES_PER_FLOAT  4L
#define PR_BYTES_PER_DOUBLE 8L
#define PR_BYTES_PER_WORD   4L
#define PR_BYTES_PER_DWORD  8L

#define PR_BITS_PER_BYTE    8L
#define PR_BITS_PER_SHORT   16L
#define PR_BITS_PER_INT     32L
#define PR_BITS_PER_INT64   64L
#define PR_BITS_PER_LONG    32L
#define PR_BITS_PER_FLOAT   32L
#define PR_BITS_PER_DOUBLE  64L
#define PR_BITS_PER_WORD    32L

#define PR_BITS_PER_BYTE_LOG2   3L
#define PR_BITS_PER_SHORT_LOG2  4L
#define PR_BITS_PER_INT_LOG2    5L
#define PR_BITS_PER_INT64_LOG2  6L
#define PR_BITS_PER_LONG_LOG2   5L
#define PR_BITS_PER_FLOAT_LOG2  5L
#define PR_BITS_PER_DOUBLE_LOG2 6L
#define PR_BITS_PER_WORD_LOG2   5L

#define PR_ALIGN_OF_SHORT   2L
#define PR_ALIGN_OF_INT     4L
#define PR_ALIGN_OF_LONG    4L
#define PR_ALIGN_OF_INT64   4L
#define PR_ALIGN_OF_FLOAT   4L
#define PR_ALIGN_OF_DOUBLE  4L
#define PR_ALIGN_OF_POINTER 4L
#define PR_ALIGN_OF_WORD    4L

#define PR_BYTES_PER_WORD_LOG2   2L
#define PR_BYTES_PER_DWORD_LOG2  3L
#define PR_WORDS_PER_DWORD_LOG2  1L

#endif
#endif /* XP_UNIX */

#endif /* nspr_cpucfg___ */
