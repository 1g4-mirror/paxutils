/* System dependent definitions for GNU tar.

   Copyright 1994-2025 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>. */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <alloca.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include <sys/stat.h>

#if !HAVE_MKFIFO && !defined mkfifo && defined S_IFIFO
# define mkfifo(file_name, mode) (mknod (file_name, (mode) | S_IFIFO, 0))
#endif

#define MODE_WXUSR	(S_IWUSR | S_IXUSR)
#define MODE_R		(S_IRUSR | S_IRGRP | S_IROTH)
#define MODE_RW		(S_IWUSR | S_IWGRP | S_IWOTH | MODE_R)
#define MODE_RWX	(S_IXUSR | S_IXGRP | S_IXOTH | MODE_RW)
#define MODE_ALL	(S_ISUID | S_ISGID | S_ISVTX | MODE_RWX)

/* Include <unistd.h> before any preprocessor test of _POSIX_VERSION.  */
#include <unistd.h>

#if MAJOR_IN_MKDEV
# include <sys/mkdev.h>
#elif defined MAJOR_IN_SYSMACROS
# include <sys/sysmacros.h>
#elif !defined major /* sys/types.h might define it */
# if MSDOS
#  define major(device)		(device)
#  define minor(device)		(device)
#  define makedev(major, minor)	(((major) << 8) | (minor))
# else
#  define major(device)		(((device) >> 8) & 0xff)
#  define minor(device)		((device) & 0xff)
#  define makedev(major, minor)	(((major) << 8) | (minor))
# endif
#endif

/* Declare wait status.  */

#include <sys/wait.h>

/* FIXME: It is wrong to use BLOCKSIZE for buffers when the logical block
   size is greater than 512 bytes; so ST_BLKSIZE code below, in preparation
   for some cleanup in this area, later.  */

/* Extract or fake data from a 'struct stat'.  ST_BLKSIZE gives the
   optimal I/O blocksize for the file, in bytes.  Some systems, like
   Sequents, return st_blksize of 0 on pipes.  */

#define DEFAULT_ST_BLKSIZE 512

#ifndef HAVE_STRUCT_STAT_ST_BLKSIZE
# define ST_BLKSIZE(statbuf) DEFAULT_ST_BLKSIZE
#else
# define ST_BLKSIZE(statbuf) \
    ((statbuf).st_blksize > 0 ? (statbuf).st_blksize : DEFAULT_ST_BLKSIZE)
#endif

/* Extract or fake data from a 'struct stat'.  ST_NBLOCKS gives the
   number of ST_NBLOCKSIZE-byte blocks in the file (including indirect blocks).
   HP-UX counts st_blocks in 1024-byte units,
   this loses when mixing HP-UX and BSD filesystems with NFS.  */

#if !HAVE_ST_BLOCKS
# define ST_NBLOCKS(statbuf) \
    ((statbuf).st_size / ST_NBLOCKSIZE \
     + ((statbuf).st_size % ST_NBLOCKSIZE != 0))
#else
# define ST_NBLOCKS(statbuf) ((statbuf).st_blocks)
# ifdef __hpux
#  define ST_NBLOCKSIZE 1024
# endif
#endif

#ifndef ST_NBLOCKSIZE
# define ST_NBLOCKSIZE 512
#endif

/* Network Appliance file systems store small files directly in the
   inode if st_size <= 64; in this case the number of blocks can be
   zero.  Perhaps other file systems have similar problems; so,
   somewhat arbitrarily, do not consider a file to be sparse if
   it has no blocks but st_size < ST_NBLOCKSIZE.  */
#define ST_IS_SPARSE(st)                                  \
  (ST_NBLOCKS (st)                                        \
   < ((st).st_size / ST_NBLOCKSIZE			  \
      + ((st).st_size % ST_NBLOCKSIZE != 0		  \
	 && (st).st_size / ST_NBLOCKSIZE != 0)))

/* Declare standard functions.  */

#include <stdckdint.h>
#include <stddef.h>
#include <stdlib.h>

#include <stdio.h>
#if !defined _POSIX_VERSION && MSDOS
# include <io.h>
#endif

#if WITH_DMALLOC
# define DMALLOC_FUNC_CHECK
# include <dmalloc.h>
#endif

#include <limits.h>
#include <inttypes.h>

/* Prototypes for external functions.  */

#include <locale.h>

#include <time.h>

/* Library modules.  */

#include <dirname.h>
#include <error.h>
#include <full-write.h>
#include <intprops.h>
#include <safe-read.h>
#include <savedir.h>
#include <unlocked-io.h>
#include <xalloc.h>

#include <gettext.h>
#define _(msgid) gettext (msgid)
#define N_(msgid) msgid

#ifdef HAVE_PWD_H
# include <pwd.h>
#endif
#ifdef HAVE_GRP_H
# include <grp.h>
#endif

#if MSDOS
# include <process.h>
# define SET_BINARY_MODE(arc) setmode(arc, O_BINARY)
# define mkdir(file, mode) (mkdir) (file)
# define TTY_NAME "con"
#else
# define SET_BINARY_MODE(arc)
# define TTY_NAME "/dev/tty"
# include <paxlib.h>
#endif
