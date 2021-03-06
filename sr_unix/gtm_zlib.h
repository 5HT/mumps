/****************************************************************
 *								*
 *	Copyright 2008 Fidelity Information Services, Inc	*
 *								*
 *	This source code contains the intellectual property	*
 *	of its copyright holder(s), and is made available	*
 *	under a license.  If you do not know the terms of	*
 *	the license, please stop and do not read further.	*
 *								*
 ****************************************************************/

#if defined(__osf__) && defined(__alpha)
	/* For some reason, zconf.h (included by zlib.h) on Tru64 seems to undefine const if STDC is not defined.
	 * The GT.M build time options currently dont define __STDC__ on Tru64 (which is what leads zconf.h to define STDC)
	 * so define STDC temporarily. In any case check if it is defined and only if not defined, do the overriding define.
	 */
#	if (!defined(STDC))
#		define	GTM_ZLIB_STDC_DEFINE
#		define	STDC
#	endif
#endif

#include <zlib.h>

#if defined(__osf__) && defined(__alpha)
	/* Undefine STDC in case it was defined just above */
#	if (defined(GTM_ZLIB_STDC_DEFINE))
#		undef STDC
#	endif
#endif

typedef int	(*zlib_cmp_func_t)(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen, int level);
typedef	int	(*zlib_uncmp_func_t)(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen);
typedef	uLong	(*zlib_cmpbound_func_t)(uLong sourceLen);

GBLREF	zlib_cmp_func_t		zlib_compress_fnptr;
GBLREF	zlib_uncmp_func_t	zlib_uncompress_fnptr;

/* The standard shared library suffix for HPUX on HPPA is .sl.
 * On HPUX/IA64, the standard suffix was changed to .so (to match other Unixes) but for
 * the sake of compatibility, they still accept (and look for) .sl if .so is not present.
 * Nevertheless, we use the standard suffix on all platforms.
 */
#if (defined(__hpux) && defined(__hppa))
#	define	ZLIB_LIBNAME	"libz.sl"
#else
#	define	ZLIB_LIBNAME	"libz.so"
#endif

#define	ZLIB_LIBFLAGS	(RTLD_NOW)	/* RTLD_NOW - resolve immediately so we know errors sooner than later */

#define	ZLIB_CMP_FNAME		"compress2"
#define	ZLIB_UNCMP_FNAME	"uncompress"

#define	ZLIB_NUM_DLSYMS		2	/* number of function names that we need to dlsym (compress2 and uncompress) */

GBLREF	int4			gtm_zlib_cmp_level;	/* zlib compression level specified at process startup */
GBLREF	int4			repl_zlib_cmp_level;	/* zlib compression level currently in use in replication pipe */

#define	ZLIB_CMPLVL_MIN		0
#define	ZLIB_CMPLVL_MAX		9	/* although currently known max zlib compression level is 9, it could be higher in
					 * future versions of zlib so we dont do any edit checks on this value inside of GT.M */
#define	ZLIB_CMPLVL_NONE	ZLIB_CMPLVL_MIN

#define	GTM_CMPLVL_OUT_OF_RANGE(x)	(ZLIB_CMPLVL_MIN > x)

void gtm_zlib_init(void);

