/****************************************************************
 *								*
 *	Copyright 2001, 2008 Fidelity Information Services, Inc	*
 *								*
 *	This source code contains the intellectual property	*
 *	of its copyright holder(s), and is made available	*
 *	under a license.  If you do not know the terms of	*
 *	the license, please stop and do not read further.	*
 *								*
 ****************************************************************/

/************************************************************************************
  *	Current GT.M operation on Unix depends on $gtm_dist environment variable
  *	being set correctly or it cannot communicate with gtmsecshr. If this
  *	communication fails, it can cause many problems.
  *	This is the startup checking including the following:
  *	gtm_chk_dist()
  *		1. $gtm_dist is defined
  *		2. $gtm_dist need to point to the current directory from which
  *			mumps and mupip were being executed from. This is verified
  *			in gtm_chk_dist() to avoid gtmsecshr forking errors later on.
  *			see C9808-000615
  *		3. $gtm_dist/gtmsecshr has setuid privs and is owned by root
  *	gtm_chk_image()
  *		4. the current image is $gtm_dist/mumps
  *	If either of these are not true, it exits.
  ************************************************************************************/
#include "mdef.h"

#include <errno.h>
#include "gtm_string.h"
#include "gtm_stat.h"
#include "gtm_stdlib.h"
#include "gtm_unistd.h"
#include "gtm_limits.h"
#include "gdsroot.h"
#include "error.h"
#include "parse_file.h"
#include "is_file_identical.h"
#include "gtm_startup_chk.h"
#include "gtmimagename.h"

GBLREF	gtmImageName		gtmImageNames[];
GBLREF	boolean_t		gtm_environment_init;
GBLREF	enum gtmImageTypes	image_type;

int gtm_chk_dist(char *image)
{
	char		*ptr1;
	char		pre_buff[MAX_FBUFF];
	char		*prefix;
	int		prefix_len;
	mstr		gtm_name;
	int		status;
	char 		mbuff[MAX_FBUFF + 1];
	parse_blk	pblk;

	error_def(ERR_DISTPATHMAX);
	error_def(ERR_SYSCALL);
	error_def(ERR_GTMDISTUNDEF);
	error_def(ERR_FILEPARSE);
	error_def(ERR_MAXGTMPATH);
	error_def(ERR_IMAGENAME);

	if (NULL != (ptr1 = (char *)GETENV(GTM_DIST)))
	{
		assert((INVALID_IMAGE != image_type) && (n_image_types > image_type));	/* assert image_type is initialized */
		if ((GTM_PATH_MAX - 2) <= (STRLEN(ptr1) + gtmImageNames[image_type].imageNameLen))
			rts_error(VARLSTCNT(3) ERR_DISTPATHMAX, 1, GTM_PATH_MAX - gtmImageNames[image_type].imageNameLen - 2);
	} else
		rts_error(VARLSTCNT(1) ERR_GTMDISTUNDEF);
	memset(&pblk, 0, sizeof(pblk));
	pblk.buffer = mbuff;
	pblk.buff_size = MAX_FBUFF;
	pblk.fop = F_SYNTAXO;
	gtm_name.addr = image;
	gtm_name.len = STRLEN(image);
	/*	strings returned in pblk are not null terminated 	*/
	status = parse_file(&gtm_name, &pblk);
	if (!(status & 1))
		rts_error(VARLSTCNT(5) ERR_FILEPARSE, 2, gtm_name.len, gtm_name.addr, status);
	assert(NULL != pblk.l_name);
	if ((!(pblk.fnb & F_HAS_DIR) && !pblk.b_dir) || (DIR_SEPARATOR != pblk.l_dir[0]))
	{
		if (NULL == GETCWD(pre_buff, MAX_FBUFF, prefix))
			rts_error(VARLSTCNT(8) ERR_SYSCALL, 5,
					LEN_AND_LIT("getcwd"), CALLFROM, errno);
		prefix_len = STRLEN(prefix);
		if (MAX_FBUFF < prefix_len + pblk.b_esl + 1)
			rts_error(VARLSTCNT(3) ERR_MAXGTMPATH, 1, MAX_FBUFF - pblk.b_name);
		if (DIR_SEPARATOR != prefix[prefix_len - 1])
		{
			prefix[prefix_len] = DIR_SEPARATOR;
			prefix[++prefix_len] = 0;
		}
		memcpy(prefix + prefix_len, pblk.l_dir, (int)pblk.b_dir);
		prefix[prefix_len + (int)pblk.b_dir] = 0;
	} else
	{
		prefix = pblk.l_dir;
		if (DIR_SEPARATOR == prefix[pblk.b_dir])
			prefix[pblk.b_dir] = 0;
	}
	if ((GTM_IMAGE == image_type) && memcmp(pblk.l_name, GTM_IMAGE_NAME, GTM_IMAGE_NAMELEN))
		rts_error(VARLSTCNT(6) ERR_IMAGENAME, 4, LEN_AND_LIT(GTM_IMAGE_NAME), pblk.b_name, pblk.l_name);
	if (GETENV("gtm_environment_init"))
		gtm_environment_init = TRUE;
	return 0;
}
