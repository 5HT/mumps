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
#define _REENTRANT

#include "mdef.h"

#include <errno.h>
#include <sys/ioctl.h>
#include <libgen.h>
#include "gtm_fcntl.h"
#include "gtm_unistd.h"
#include "gtm_stat.h"
#include "gtm_stdio.h"
#include "gtm_string.h"
#include "gtm_stdlib.h"
#include "gtm_limits.h"

#include "stringpool.h"
#include "io.h"
#include "iormdef.h"
#include "io_params.h"
#include "eintr_wrappers.h"

LITREF	unsigned char		io_params_size[];
GBLREF io_pair			io_curr_device;
GBLREF io_log_name 		*io_root_log_name;

#define FREE_ALL { if (NULL != copy_cmd_string) free(copy_cmd_string); if (NULL != temp) free(temp);\
		if (NULL != buf)  free(buf); if (NULL != dir_in_path) free(dir_in_path);if (NULL != command2) free(command2); }

int parse_pipe(char *cmd_string, char *ret_token);

/* The parse_pipe routine is to used to determine if all the commands in a pipe string are valid commands so
   the iopi_open() routine can detect the problem prior to a fork.

   Command Parsing Limitations:

   All commands will be searched for in $PATH and in $gtm_dist and if not found will trap.  Most pipe strings can be
   entered with some minor exceptions.  First, no parentheses around commands are allowed. An example of this which would
   fail is:

	o p:(comm="(cd; pwd)":writeonly)::"pipe"

   This produces nothing different from:

	o p:(comm="cd; pwd":writeonly)::"pipe"

   This restriction does not include parentheses embedded in character strings as in:

	o p:(comm="echo ""(test)""":writeonly)::"pipe"       (which would output: (test) )

   or as parameters to a command as in:

	o p:(comm="tr -d '()'":writeonly)::"pipe"     (which if passed (test) would output: test )

   Second, commands must be valid commands - not aliases.  Third, no shell built-in commands are allowed (unless a version
   is found in the $PATH and $gtm_dist); with the exception of nohup and cd.  In the case of nohup, the next token will be
   the command to be looked for in $PATH and $gtm_dist.  "cd" will be allowed, but no parsing for a command will occur
   until the next "|" token - if there is one.  In addition, environment variables may be used to define a path, or actual
   paths may be given.

   The following are examples of valid open commands:

   1.   o a:(comm="tr e j | echoback":stderr=e:exception="g BADOPEN")::"pipe"
   2.   o a:(comm="/bin/cat |& nl")::"pipe"
   3.   o a:(comm="mupip integ mumps.dat")::"pipe"
   4.   o a:(comm="$gtm_dist/mupip integ mumps.dat")::"pipe"
   5.   o a:(comm="nohup cat")::"pipe"
   6.   o p:(comm="cd ..; pwd | tr -d e":writeonly)::"pipe"

   In the first example the commands parsed are "tr" and "echoback".  "echoback" is in the current directory which is
   included in the $PATH.  In the second example an explicit path is given for "cat", but "nl" is found via $PATH.
   In the third example mupip will be found if it is in the $PATH or in $gtm_dist.  In the fourth example the
   $gtm_dist environment variable is explicitly used to find mupip.  In the fifth example the "cat" after "nohup"
   is used as the command looked up in $PATH.  In the sixth example the default directory is moved up a level and
   pwd is executed with the output piped to the tr command.  The pwd command is not checked for existence, but the
   "tr" command after the "|" is checked.
*/
int parse_pipe(char *cmd_string, char *ret_token)
{
	char *str1, *str2, *str3, *token;
	char *saveptr1, *saveptr2, *saveptr3;
	char *env_var;
	int env_inc;
	struct stat sb;
	char *path;
	char *temp;
	char *buf;
	char *dir_in_path;
	char *command2;
	char *copy_cmd_string;
	char *token1;
	char *token2;
	char *token3;
	int notfound = FALSE;
	int ret_stat;
	int pathsize;
	int cmd_string_size;

	path = GETENV("PATH");

	cmd_string_size = STRLEN(cmd_string) + 1;
	pathsize = PATH_MAX + cmd_string_size;
	buf = (char *)malloc(pathsize);
	copy_cmd_string = (char *)malloc(cmd_string_size);
	dir_in_path = (char *)malloc(PATH_MAX);
	command2 = (char *)malloc(cmd_string_size);
	temp = (char *)malloc(pathsize);
	memcpy(copy_cmd_string, cmd_string, cmd_string_size);

	/* guaranteed at least one token when we get here because it is checked right after iop_eol loop and
	 before this code is executed. */
	for (str1 = copy_cmd_string; FALSE == notfound ; str1 = NULL)
	{
		/* separate into tokens in a pipe */
		token1 = strtok_r(str1, "|", &saveptr1);
		if (NULL == token1)
			break;

		/* separate into tokens using space as delimiter */
		/* if the first token is a non-alpha-numeric or if it is nohup the skip it */
		/* and use the next one as a command to find */
		memcpy(command2, token1, STRLEN(token1) + 1);

		for (str2 = command2; ; str2 = NULL)
		{
			token2 = strtok_r(str2, " >&;",&saveptr2);
			if (NULL != token2 && !strcmp(token2, "cd"))
			{
				/* if the command is cd then skip the rest before the next pipe */
				token2 = NULL;
				break;
			}
			if (NULL != token2 && strcmp(token2, "nohup"))
				break;
			if (NULL == token2)
				break;
		}

		if (NULL == token2)
			continue;

		notfound = TRUE;
		if (NULL != strchr(token2, '/'))
		{
			/* if the first character is a $ sign then assume it is an environment variable used
			   like $gtm_dist/mupip.  Get the environment variable and substitute it. */
			notfound = TRUE;
			if ('$' == *token2)
			{
				for (env_inc = 1; '/' != *(token2 + env_inc); env_inc++)
				{
					temp[env_inc - 1] = *(token2 + env_inc);
				}
				temp[env_inc] = '\0';
				env_var = GETENV(temp);
				if (NULL != env_var)
				{
					/* build a translated path to command */
					assert(cmd_string_size > (STRLEN(token2 + env_inc) + STRLEN(env_var)));
					SPRINTF(temp, "%s%s", env_var, token2 + env_inc);

					/* The command must be a regular file and executable */
					STAT_FILE(temp, &sb, ret_stat);
					if (0 == ret_stat && (S_ISREG(sb.st_mode)) && (sb.st_mode & (S_IXOTH | S_IXGRP | S_IXUSR)))
						notfound = FALSE;
				}
			} else
			{
				STAT_FILE(token2, &sb, ret_stat);
				if (0 == ret_stat && (S_ISREG(sb.st_mode)) && (sb.st_mode & (S_IXOTH | S_IXGRP | S_IXUSR)))
					notfound = FALSE;
			}
		} else
		{
			/* look in $gtm_dist in case not explicitly listed or not in the $PATH variable */
			env_var = GETENV("gtm_dist");
			if (NULL != env_var)
			{
				/* build a translated path to command */
				SPRINTF(temp, "%s/%s", env_var, token2);
				STAT_FILE(temp, &sb, ret_stat);
				if (0 == ret_stat && (S_ISREG(sb.st_mode)) && (sb.st_mode & (S_IXOTH | S_IXGRP | S_IXUSR)))
					notfound = FALSE;
				else
					notfound = TRUE;
			}

			/* search all the directories in the $PATH */
			memcpy(dir_in_path, path, STRLEN(path) + 1);
			for (str3= dir_in_path; TRUE == notfound; str3 = NULL)
			{
				token3 = strtok_r(str3, ":", &saveptr3);
				if (NULL == token3)
					break;
				SPRINTF(buf, "%s/%s", token3, token2);
				notfound = TRUE;
				/* The command must be a regular file and executable */
				STAT_FILE(buf, &sb, ret_stat);
				if (0 == ret_stat && (S_ISREG(sb.st_mode)) && (sb.st_mode & (S_IXOTH | S_IXGRP | S_IXUSR)))
				{
					notfound = FALSE;
					break;
				}
			}
		}
		if (TRUE == notfound)
		{
			assert(PATH_MAX > (STRLEN(token2) + 1));
			memcpy(ret_token, token2, STRLEN(token2) + 1);
			FREE_ALL;
			return(FALSE);
		}
	}
	FREE_ALL;
	return(TRUE);
}

/* When we are in this routine dev_name is same as naml in io_open_try() */

#define PIPE_ERROR_INIT()\
{\
	if (NULL != pcommand) free(pcommand);\
	if (NULL != pshell) free(pshell);\
	if (NULL != pstderr) free(pstderr);\
	iod->type = rm;\
}

short iopi_open(io_log_name *dev_name, mval *pp, int fd, mval *mspace, int4 timeout)
{
	io_desc		*iod;
	io_desc		*stderr_iod;
 	d_rm_struct	*d_rm;
	io_log_name	*stderr_naml;				/* logical record for stderr device */
	error_def(ERR_SYSCALL);

	unsigned char	ch;
	int		param_cnt = 0;
	int		p_offset = 0;
	int		file_des_write;
	int		file_des_read = 0;
	int		file_des_read_stderr;
	int		param_offset = 0;
	struct stat 	sb;
	int 		ret_stat;
	int 		cpid;
	int 		pfd_write[2];
	int 		pfd_read[2];
	int 		pfd_read_stderr[2];
	enum 		pfield {PSHELL,PCOMMAND,PSTDERR};
	int		slen[3] = {0, 0, 0};
	char 		*sparams[3];
	char 		*pcommand = 0;
	char 		*pshell = 0;
	char 		*pshell_name;
	char 		*pstderr = 0;
	char 		*sh;
	int		independent = FALSE;
	int		parse = FALSE;
	int 		status_read;
	int		return_stdout = TRUE;
	int		return_stderr = FALSE;
	char		ret_token[GTM_MAX_DIR_LEN];
	int		save_errno;
	int		flags;
	int		fcntl_res;

	error_def(ERR_DEVOPENFAIL);
	error_def(ERR_TEXT);

	iod = dev_name->iod;

	while (iop_eol != *(pp->str.addr + p_offset))
	{
		assert((params) *(pp->str.addr + p_offset) < (params)n_iops);
		switch ((ch = *(pp->str.addr + p_offset++)))
		{
			case iop_exception:
				dev_name->iod->error_handler.len = *(pp->str.addr + p_offset);
				dev_name->iod->error_handler.addr = (char *)(pp->str.addr + p_offset + 1);
				s2pool(&dev_name->iod->error_handler);
				break;
			case iop_shell:
				slen[PSHELL] = (unsigned int)(unsigned char)*(pp->str.addr + p_offset);
				sparams[PSHELL] = (char *)(pp->str.addr + p_offset + 1);
				param_cnt++;
				break;
			case iop_command:
				slen[PCOMMAND] = (unsigned int)(unsigned char)*(pp->str.addr + p_offset);
				sparams[PCOMMAND] = (char *)(pp->str.addr + p_offset + 1);
				param_cnt++;
				break;
			case iop_stderr:
				slen[PSTDERR] = (unsigned int)(unsigned char)*(pp->str.addr + p_offset);
				sparams[PSTDERR] = (char *)(pp->str.addr + p_offset + 1);
				param_cnt++;
				break;
			case iop_independent:
				independent = TRUE;
				break;
			case iop_parse:
				parse = TRUE;
				break;
			case iop_writeonly:
				return_stdout = FALSE;
				break;
			default:
				break;
		}
		p_offset += ((IOP_VAR_SIZE == io_params_size[ch]) ?
			     (unsigned char)*(pp->str.addr + p_offset) + 1 : io_params_size[ch]);
	}

	/* If command device parameter not entered then exit with error */
	if (0 == slen[PCOMMAND])
	{
		PIPE_ERROR_INIT();
		rts_error(VARLSTCNT(4) ERR_DEVOPENFAIL, 2, LEN_AND_LIT("PIPE - No command string entered"));
	} else
	{
		pcommand = malloc(slen[PCOMMAND] + 1);
		memcpy(pcommand, sparams[PCOMMAND], slen[PCOMMAND]);
		pcommand[slen[PCOMMAND]] = '\0';
		if (TRUE == parse)
		{
			if (FALSE == parse_pipe(pcommand, ret_token))
			{
				PIPE_ERROR_INIT();
				rts_error(VARLSTCNT(8) ERR_DEVOPENFAIL, 2, LEN_AND_STR(ret_token),
					  ERR_TEXT, 2, LEN_AND_LIT("Invalid command string"));
			}
		}
	}

	/* check the shell device parameter before the fork/exec */
	if (0 != slen[PSHELL])
	{
		pshell = malloc(slen[PSHELL] + 1);
		memcpy(pshell, sparams[PSHELL], slen[PSHELL]);
		pshell[slen[PSHELL]] = '\0';
		/* The shell must be a regular file and executable */
		STAT_FILE(pshell, &sb, ret_stat);
		if (-1 == ret_stat || !(S_ISREG(sb.st_mode)) || !(sb.st_mode & (S_IXOTH | S_IXGRP | S_IXUSR)))
		{
			save_errno = errno;
			assert(GTM_MAX_DIR_LEN - 1 >= STRLEN(pshell));
			STRCPY(ret_token,pshell);
			PIPE_ERROR_INIT();
			rts_error(VARLSTCNT(8) ERR_DEVOPENFAIL, 2, LEN_AND_STR(ret_token),
				  ERR_TEXT, 2, LEN_AND_LIT("Invalid shell"));
		}
		pshell_name = basename(pshell);
	}

	/* check the stderr device parameter before the fork/exec */
	if (0 != slen[PSTDERR])
	{
		pstderr = malloc(slen[PSTDERR] + 1);
		memcpy(pstderr, sparams[PSTDERR], slen[PSTDERR]);
		pstderr[slen[PSTDERR]] = '\0';
		return_stderr = TRUE;
	}

	/* child to read from pfd_write[0] and parent to write to pfd_write[1] */
	if (-1 == pipe(pfd_write))
	{
		save_errno = errno;
		PIPE_ERROR_INIT();
		rts_error(VARLSTCNT(5) ERR_DEVOPENFAIL, 2, LEN_AND_LIT("PIPE - pipe(pfd_write) failed"), save_errno);
	} else
		iod->dollar.zeof = FALSE;

	/* child to write stdout (and possibly stderr) to pfd_read[1] and parent to read from pfd_read[0] */
	if (return_stdout)
		if (-1 == pipe(pfd_read))
		{
			save_errno = errno;
			PIPE_ERROR_INIT();
			rts_error(VARLSTCNT(5) ERR_DEVOPENFAIL, 2, LEN_AND_LIT("PIPE - pipe(pfd_read) failed"), save_errno);
		} else
			file_des_read = pfd_read[0];

	/* child to write to pfd_read_stderr[1] and parent to read from pfd_read_stderr[0] */
	if (return_stderr)
		if (-1 == pipe(pfd_read_stderr))
		{
			save_errno = errno;
			PIPE_ERROR_INIT();
			rts_error(VARLSTCNT(5) ERR_DEVOPENFAIL, 2, LEN_AND_LIT("PIPE - pipe(pfd_read_stderr) failed"), save_errno);
		} else
			file_des_read_stderr = pfd_read_stderr[0];

	file_des_write = pfd_write[1];
	/*do the fork and exec */
	cpid = fork();
	if (-1 == cpid)
	{
		save_errno = errno;
		PIPE_ERROR_INIT();
		rts_error(VARLSTCNT(5) ERR_DEVOPENFAIL, 2, LEN_AND_LIT("PIPE - fork() failed"), save_errno);
	}
	if (0 == cpid)
	{
		/* in child */
		int ret;
		io_desc *iod;
		io_log_name	*l, *prev;
		d_rm_struct	*d_rm;

		close(pfd_write[1]);          /* Close unused write end */
		if (return_stdout)
			close(pfd_read[0]);          /* Close unused read end for stdout return*/
		if (return_stderr)
			close(pfd_read_stderr[0]);          /* Close unused read end for stderr return */
		/* close any other pipe output file_des_write */
		assert (0 != io_root_log_name);
		assert(0 == io_root_log_name->len);
		for (prev = io_root_log_name, l = prev->next;  0 != l;  prev = l, l = l->next)
		{
			iod = l->iod;
			d_rm = (d_rm_struct *) iod->dev_sp;
			if (d_rm && TRUE == d_rm->pipe)
				close(d_rm->fildes);
		}

		close(0);
		/* stdin becomes pfd_write[0] */
		if (-1 == dup2(pfd_write[0], 0))
		{
			save_errno = errno;
			PIPE_ERROR_INIT();
			rts_error(VARLSTCNT(5) ERR_DEVOPENFAIL, 2,
				  LEN_AND_LIT("PIPE - dup2(pfd_write[0]) failed in child"), save_errno);
		}

		if (return_stdout)
		{
			/* stdout becomes pfd_read[1] */
			close(1);
			if (-1 == dup2(pfd_read[1], 1))
			{
				save_errno = errno;
				PIPE_ERROR_INIT();
				rts_error(VARLSTCNT(5) ERR_DEVOPENFAIL, 2,
					  LEN_AND_LIT("PIPE - dup2(pfd_read[1],1) failed in child"), save_errno);
			}

			/* stderr also becomes pfd_read[1] if return_stderr is false*/
			if (FALSE == return_stderr)
			{
				close(2);
				if (-1 == dup2(pfd_read[1], 2))
				{
					save_errno = errno;
					PIPE_ERROR_INIT();
					rts_error(VARLSTCNT(5) ERR_DEVOPENFAIL, 2,
						  LEN_AND_LIT("PIPE - dup2(pfd_read[1],2) failed in child"), save_errno);
				}
			}
		}

		if (return_stderr)
		{
			close(2);
			if (-1 == dup2(pfd_read_stderr[1], 2))
			{
				save_errno = errno;
				PIPE_ERROR_INIT();
				rts_error(VARLSTCNT(5) ERR_DEVOPENFAIL, 2,
					  LEN_AND_LIT("PIPE - dup2(pfd_read_stderr[1],2) failed in child"), save_errno);
			}
		}

		if (0 == slen[PSHELL])
		{
			/* get SHELL environment */
			sh = GETENV("SHELL");
			/* use bourne shell as default if SHELL not set in environment*/
			if (!sh)
			{
				ret = EXECL("/bin/sh", "sh", "-c", pcommand, (char *)NULL);
			} else
				ret = EXECL(sh, basename(sh), "-c", pcommand, (char *)NULL);
		} else
			ret = EXECL(pshell, pshell_name, "-c", pcommand, (char *)NULL);

		if (-1 == ret)
		{
			save_errno = errno;
			PIPE_ERROR_INIT();
			rts_error(VARLSTCNT(5) ERR_DEVOPENFAIL, 2, LEN_AND_LIT("PIPE - execl failed in child"), save_errno);
		}
	} else
	{
		/* in parent */
		close(pfd_write[0]);          /* Close unused read end */
		/* if returning stdout then close unused write end */
		if (return_stdout)
			close(pfd_read[1]);
		if (return_stderr)
			close(pfd_read_stderr[1]);
	}
	assert((params) *(pp->str.addr) < (unsigned char) n_iops);
	assert(0 != iod);
	assert(0 <= iod->state && iod->state < n_io_dev_states);
	assert(pi == iod->type);
	if (!(d_rm = (d_rm_struct *) iod->dev_sp))
	{	iod->dev_sp = (void*)malloc(SIZEOF(d_rm_struct));
		memset(iod->dev_sp, 0, SIZEOF(d_rm_struct));
		d_rm = (d_rm_struct *) iod->dev_sp;
		/* save child's process id for reaping on close */
		d_rm->pipe_pid = cpid;
		/* save read file descriptor for stdout return if set */
		if (file_des_read)
		{
			if (NULL == (d_rm->read_filstr = FDOPEN(file_des_read, "r")))
			{
				save_errno = errno;
				PIPE_ERROR_INIT();
				rts_error(VARLSTCNT(9) ERR_DEVOPENFAIL, 2, dev_name->len, dev_name->dollar_io,
					  ERR_TEXT, 2, LEN_AND_LIT("Error in stream open"), save_errno);
			}
			d_rm->read_fildes = file_des_read;
		}
		SPRINTF(&d_rm->dollar_key[0], "%d", cpid); /* save in pipe specific structure for $KEY access */
		memcpy(d_rm->dollar_device, "0", SIZEOF("0"));
		iod->state = dev_closed;
                d_rm->stream = FALSE;
                iod->width = DEF_RM_WIDTH;
                iod->length = DEF_RM_LENGTH;
		d_rm->independent = independent;
		d_rm->parse = parse;
		d_rm->recordsize = DEF_RM_RECORDSIZE;
		d_rm->def_width = d_rm->def_recsize = TRUE;
                d_rm->fixed = FALSE;
                d_rm->noread = FALSE;
		d_rm->padchar = DEF_RM_PADCHAR;
		d_rm->inbuf = NULL;
		d_rm->outbuf = NULL;
		d_rm->dev_param_pairs.num_pairs = param_cnt;
		if (slen[PSHELL])
		{
			d_rm->dev_param_pairs.pairs[param_offset].name = malloc(SIZEOF("SHELL="));
			strcpy(d_rm->dev_param_pairs.pairs[param_offset].name, "SHELL=");
			/* add quotes around the command field */
			d_rm->dev_param_pairs.pairs[param_offset].definition = malloc(STRLEN(pshell) + 5);
			SPRINTF(d_rm->dev_param_pairs.pairs[param_offset++].definition, "\"%s\"", pshell);
			if (NULL != pshell)
				free(pshell);
		}

		/* We checked for command existence earlier so no need to check again */
		d_rm->dev_param_pairs.pairs[param_offset].name = malloc(SIZEOF("COMMAND="));
		strcpy(d_rm->dev_param_pairs.pairs[param_offset].name, "COMMAND=");
		/* add quotes around the command field */
		d_rm->dev_param_pairs.pairs[param_offset].definition = malloc(STRLEN(pcommand) + 5);
		SPRINTF(d_rm->dev_param_pairs.pairs[param_offset++].definition, "\"%s\"", pcommand);
		if (NULL != pcommand)
			free(pcommand);

		if (slen[PSTDERR])
		{
			d_rm->dev_param_pairs.pairs[param_offset].name = malloc(SIZEOF("STDERR="));
			strcpy(d_rm->dev_param_pairs.pairs[param_offset].name, "STDERR=");
			/* add quotes around the stderr field */
			d_rm->dev_param_pairs.pairs[param_offset].definition = malloc(STRLEN(pstderr) + 5);
			SPRINTF(d_rm->dev_param_pairs.pairs[param_offset].definition, "\"%s\"", pstderr);
			if (NULL != pstderr)
				free(pstderr);
		}
	}
	d_rm->pipe = TRUE;
	iod->type = rm;

	if (return_stderr)
	{
		/* We will create an mstr (using MSTR_DEF in mdef.h) and call get_log_name(..,INSERT) like
		   op_open does to create and insert io_log_name * in io_root_log_name. */


		MSTR_DEF(stderr_mstr, slen[PSTDERR], sparams[PSTDERR]);
		stderr_naml = get_log_name(&stderr_mstr, INSERT);

		stderr_iod = stderr_naml->iod =  (io_desc *)malloc(SIZEOF(io_desc));
		memset((char*)stderr_naml->iod, 0, SIZEOF(io_desc));
		stderr_naml->iod->pair.in  = stderr_naml->iod;
		stderr_naml->iod->pair.out = stderr_naml->iod;
		stderr_naml->iod->trans_name = stderr_naml;
		(stderr_iod->pair.in)->trans_name = stderr_naml;
		{
			io_desc		*io_ptr;
			d_rm_struct	*in_d_rm;
			io_log_name	dev_name;	/*	dummy	*/

			io_ptr = ( stderr_iod->pair.in);
			if (!(in_d_rm = (d_rm_struct *) io_ptr->dev_sp))
			{
				io_ptr->dev_sp = (void*)malloc(SIZEOF(d_rm_struct));
				memset(io_ptr->dev_sp, 0, SIZEOF(d_rm_struct));
				in_d_rm = (d_rm_struct *) io_ptr->dev_sp;
				memcpy(in_d_rm->dollar_device, "0", SIZEOF("0"));
				io_ptr->state = dev_closed;
				in_d_rm->stream = d_rm->stream;
				io_ptr->width = iod->width;
				io_ptr->length = iod->length;
				io_ptr->wrap = DEFAULT_IOD_WRAP;
				io_ptr->dollar.x = 0;
				io_ptr->dollar.y = 0;
				io_ptr->dollar.za = 0;
				io_ptr->dollar.zb[0] = 0;
				io_ptr->disp_ptr = iod->disp_ptr;
				in_d_rm->fixed = d_rm->fixed;
				in_d_rm->noread = TRUE;
				in_d_rm->padchar = d_rm->padchar;
				in_d_rm->inbuf = d_rm->inbuf;
				in_d_rm->outbuf = d_rm->outbuf;
				in_d_rm->stderr_parent = iod;
			}
			in_d_rm->pipe = TRUE;
			io_ptr->type = rm;
			status_read = iorm_open(stderr_naml, pp, file_des_read_stderr, mspace, timeout);
			if (TRUE == status_read)
				( stderr_iod->pair.in)->state = dev_open;
			else if (dev_open == ( stderr_iod->pair.out)->state)
				( stderr_iod->pair.in)->state = dev_closed;
		}
		d_rm->stderr_child = stderr_iod;
	}
	flags = 0;
	FCNTL2(file_des_write, F_GETFL, flags);
	if (0 > flags)
		rts_error(VARLSTCNT(8) ERR_SYSCALL, 5, LEN_AND_LIT("fcntl"), CALLFROM, errno);
	FCNTL3(file_des_write, F_SETFL, (flags | O_NONBLOCK), fcntl_res);
	if (0 > fcntl_res)
		rts_error(VARLSTCNT(8) ERR_SYSCALL, 5, LEN_AND_LIT("fcntl"), CALLFROM, errno);
	return iorm_open(dev_name, pp, file_des_write, mspace, timeout);
}
