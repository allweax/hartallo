#ifndef _HARTALLO_CONFIG_H_
#define _HARTALLO_CONFIG_H_

#ifdef __SYMBIAN32__
#undef _WIN32 /* Because of WINSCW */
#endif

/* Windows (XP/Vista/7/CE and Windows Mobile) macro definition. */
#if defined(WIN32)|| defined(_WIN32) || defined(_WIN32_WCE)
#	define HL_UNDER_WINDOWS	1
#	define HL_STDCALL __stdcall
#	if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP || WINAPI_FAMILY == WINAPI_FAMILY_APP)
#		define HL_UNDER_WINDOWS_RT		1
#	endif
#else
#	define HL_STDCALL
#endif

/* OS X or iOS */
#if defined(__APPLE__)
#	define HL_UNDER_APPLE				1
#endif
#if TARGET_OS_MAC
#	define HL_UNDER_MAC				1
#endif
#if TARGET_OS_IPHONE
#	define HL_UNDER_IPHONE			1
#endif
#if TARGET_IPHONE_SIMULATOR
#	define HL_UNDER_IPHONE_SIMULATOR	1
#endif

/* Used on Windows and Symbian systems to export/import public functions and global variables. */
#if !defined(__GNUC__) && defined(HARTALLO_EXPORTS)
# 	define HARTALLO_API		__declspec(dllexport)
#	define HARTALLO_GEXTERN	extern __declspec(dllexport)
#	define HARTALLO_EXPORT	extern __declspec(dllexport)
#elif !defined(__GNUC__) && !defined(HARTALLO_IMPORTS_IGNORE)
# 	define HARTALLO_API		__declspec(dllimport)
#	define HARTALLO_GEXTERN	__declspec(dllimport)
#	define HARTALLO_EXPORT
#else
#	define HARTALLO_API
#	define HARTALLO_GEXTERN	extern
#	define HARTALLO_EXPORT
#endif

/* Guards against C++ name mangling */
#ifdef __cplusplus
#	define HL_BEGIN_DECLS extern "C" {
#	define HL_END_DECLS }
#else
#	define HL_BEGIN_DECLS
#	define HL_END_DECLS
#endif

/* "inline" definition */

#if defined(_MSC_VER)
#	define HL_INLINE	__forceinline
#elif defined(__GNUC__)
#	define HL_INLINE	__inline
#else
#	define HL_INLINE
#endif

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86) // FIXME: ARM64
#	define HL_CPU_TYPE_X86		1
#endif
#if defined(__x86_64__) || defined(_M_X64) || defined(__LP64__)
#	define HL_CPU_TYPE_X64		1
#endif

#if HL_CPU_TYPE_X64
#	define HL_ALIGN_V	32
#elif HL_CPU_TYPE_X86
#	define HL_ALIGN_V	16
#else
#	define HL_ALIGN_V		16
#endif

#if defined(_MSC_VER)
#	define _CRT_SECURE_NO_WARNINGS
#	define HL_SHOULD_INLINE	__inline
#	define HL_ALWAYS_INLINE	__forceinline
#	define HL_ALIGN(x)		__declspec(align(x))
#	define HL_NAKED			__declspec(naked)
#	pragma warning( disable : 4996 )
#	include <intrin.h>
#elif defined(__GNUC__)
#	define HL_ALWAYS_INLINE __inline __attribute__ ((__always_inline__))
#	define HL_SHOULD_INLINE	inline
#	define HL_ALIGN(x)		__attribute__((aligned(x)))
#	define HL_NAKED			__attribute__((naked))
#	if HL_CPU_TYPE_X86
#	include <x86intrin.h>
#	endif /* HL_CPU_TYPE_X86 */
#else
#	define HL_ALWAYS_INLINE	inline
#	define HL_SHOULD_INLINE	inline
#	define HL_ALIGN(x)
#endif
#define HL_ALIGNED(x)

#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

#if HAVE_CONFIG_H
#	include <config.h>
#endif

// Useless but give us short hand for quick testing
#if !defined(HL_DISABLE_INTRIN)
#	define HL_DISABLE_INTRIN 0
#endif /* HL_DISABLE_INTRIN */
#if !defined (HL_DISABLE_ASM)
#	define HL_DISABLE_ASM 0
#endif /* HL_DISABLE_ASM */

#if !defined(HL_HAVE_X86_INTRIN)
#	define HL_HAVE_X86_INTRIN		(HL_CPU_TYPE_X86 && !HL_DISABLE_INTRIN)
#endif /* HL_HAVE_X86_INTRIN */
#if !defined(HL_HAVE_X86_ASM)
#	define HL_HAVE_X86_ASM			(HL_CPU_TYPE_X86 && !HL_DISABLE_ASM && !HL_CPU_TYPE_X64) // FIXME: !HL_CPU_TYPE_X64
#endif /* HL_HAVE_X86_ASM */

#endif /* _HARTALLO_CONFIG_H_ */
