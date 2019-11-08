#ifndef UTILITY_HPP
#define UTILITY_HPP

#ifdef UNICODE
#   ifndef _UNICODE
#       define _UNICODE
#   endif
#endif

#ifdef _UNICODE
#   ifndef UNICODE
#       define UNICODE
#   endif
#endif

#ifdef NTDDI_VERSION
#undef NTDDI_VERSION
#endif
#define NTDDI_VERSION                   0x06020000  /* NTDDI_WIN8 */

#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT                    0x0602  /* _WIN32_WINNT_WIN8 */

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

#if defined(_DEBUG) && !defined(NDEBUG)
#   define DEBUG
#elif !defined(_DEBUG) && defined(NDEBUG)
#   define RELEASE
#else
#   error Use either -D_DEBUG or -DNDEBUG parameters during compilation.
#endif

#include "exception.hpp"

#ifdef DEBUG
#   ifdef ASSERT
#       error ASSERT should not be defined yet.
#   endif
#endif

#ifndef ASSERT
#   define ASSERT(ex, fmt, ...)         if (!(ex)) { throw Exception(__LINE__, __FILE__, fmt, ##__VA_ARGS__); }
#endif

#ifndef VERBOSE
#   ifdef DEBUG
#       define VERBOSE(fmt, ...)        fprintf(stderr, fmt, ##__VA_ARGS__)
#   else
#       define VERBOSE(fmt, ...)        /* Not in RELEASE mode */
#   endif
#endif

#ifndef ALIGN
#   define ALIGN(p, x)                  ((p + (x-1)) & (~(x-1)))
#endif

#ifndef ASSET_PROPERTIES
#   ifdef DEBUG
#       define ASSET_PROPERTIES         public
#   else
#       define ASSET_PROPERTIES         private
#   endif
#endif

/** Used within the fast files. */
typedef unsigned int address_t;

#endif /* UTILITY_HPP */
