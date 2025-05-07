/* Definitions for communicating with a remote tape drive.

   Copyright (C) 1988, 1992, 1996-1997, 2001, 2003-2004, 2007, 2023-2025 Free
   Software Foundation, Inc.

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

extern char const *rmt_command;

int rmt_open__ (const char *, int, int, const char *);
int rmt_close__ (int);
ptrdiff_t rmt_read__ (int, void *, idx_t);
idx_t rmt_write__ (int, void const *, idx_t);
off_t rmt_lseek__ (int, off_t, int);
int rmt_ioctl__ (int, unsigned long int, void *);

extern bool force_local_option;

_GL_INLINE_HEADER_BEGIN
#ifndef RMT_INLINE
# define RMT_INLINE _GL_INLINE
#endif

/* A filename is remote if it contains a colon not preceded by a slash,
   to take care of `/:/' which is a shorthand for `/.../<CELL-NAME>/fs'
   on machines running OSF's Distributing Computing Environment (DCE) and
   Distributed File System (DFS).  However, when --force-local, a
   filename is never remote.  */

RMT_INLINE bool
_remdev (char const *__dev_name)
{
  if (force_local_option || *__dev_name == ':')
    return false;
  for (char const *__p = __dev_name; *__p && (*__p != '/'); __p++)
    if (*__p == ':')
      return true;
  return false;
}

enum { __REM_BIAS = 1 << 30 };

RMT_INLINE bool
_isrmt (int __fd)
{
  return __REM_BIAS <= __fd;
}

RMT_INLINE int
rmtopen (char const *__name, int __flags, mode_t __mode, char const *__command)
{
  return (_remdev (__name)
	  ? rmt_open__ (__name, __flags, __REM_BIAS, __command)
	  : open (__name, __flags, __mode));
}

RMT_INLINE int
rmtcreat (char const *__name, mode_t __mode, char const *__command)
{
  return rmtopen (__name, O_CREAT | O_WRONLY | O_TRUNC, __mode, __command);
}

RMT_INLINE ptrdiff_t
rmtread (int __fd, void *__buffer, idx_t __length)
{
  return (_isrmt (__fd)
	  ? rmt_read__ (__fd - __REM_BIAS, __buffer, __length)
	  : safe_read (__fd, __buffer, __length));
}

RMT_INLINE idx_t
rmtwrite (int __fd, void const *__buffer, idx_t __length)
{
  return (_isrmt (__fd)
	  ? rmt_write__ (__fd - __REM_BIAS, __buffer, __length)
	  : full_write (__fd, __buffer, __length));
}

RMT_INLINE off_t
rmtlseek (int __fd, off_t __offset, int __whence)
{
  return (_isrmt (__fd)
	  ? rmt_lseek__ (__fd - __REM_BIAS, __offset, __whence)
	  : lseek (__fd, __offset, __whence));
}

RMT_INLINE int
rmtclose (int __fd)
{
  return _isrmt (__fd) ? rmt_close__ (__fd - __REM_BIAS) : close (__fd);
}

RMT_INLINE int
rmtioctl (int __fd, unsigned long int __request, void *__argument)
{
  return (_isrmt (__fd)
	  ? rmt_ioctl__ (__fd - __REM_BIAS, __request, __argument)
	  : ioctl (__fd, __request, __argument));
}

_GL_INLINE_HEADER_END
