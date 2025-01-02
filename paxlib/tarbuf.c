/* This file is part of GNU paxutils

   Copyright (C) 2005, 2007, 2023-2025 Free Software Foundation, Inc.

   Written by Sergey Poznyakoff

   GNU paxutils is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 3, or (at your option) any later
   version.

   GNU paxutils is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
   Public License for more details.

   You should have received a copy of the GNU General Public License along
   with GNU paxutils.  If not, see <http://www.gnu.org/licenses/>. */

#include <system.h>
#include <safe-read.h>
#include <safe-write.h>
#include <paxbuf.h>
#include <pax.h>
#include <tar.h>

typedef struct tar_archive
{
  char *filename;           /* Name of the archive file */
  int fd;                   /* Archive file descriptor */
  idx_t bfactor;	    /* Number of blocks in a record */
  const char *rsh;          /* Full pathname of rsh */
  const char *rmt;          /* Full pathname of the remote command */
}
tar_archive_t;


/* Operations on local files */

static pax_io_status_t
local_reader (void *closure, void *data, idx_t size, idx_t *ret_size)
{
  tar_archive_t *tar = closure;
  ssize_t s = read (tar->fd, data, size);
  *ret_size = s + (s < 0);
  return s < 0 ? pax_io_failure : s == 0 ? pax_io_eof : pax_io_success;
}

static pax_io_status_t
local_writer (void *closure, void *data, idx_t size, idx_t *ret_size)
{
  tar_archive_t *tar = closure;
  ssize_t s = write (tar->fd, data, size);
  *ret_size = s + (s < 0);
  return s < 0 ? pax_io_failure : pax_io_success;
}

static int
local_seek (void *closure, off_t offset)
{
  tar_archive_t *tar = closure;
  off_t off;

  off = lseek (tar->fd, offset, SEEK_SET);
  if (off == -1)
    return pax_io_failure;
  return pax_io_success;
}

static int
local_open (void *closure, int pax_mode)
{
  tar_archive_t *tar = closure;
  int mode = (pax_mode & PAXBUF_READ) ? O_RDONLY :
              O_RDWR | ((pax_mode & PAXBUF_CREAT) ? O_CREAT : 0);
  tar->fd = open (tar->filename, mode, MODE_RW);
  if (tar->fd == -1)
    return pax_io_failure;
  return pax_io_success;
}

static int
local_close (void *closure, int mode)
{
  tar_archive_t *tar = closure;
  close (tar->fd);
  tar->fd = -1;
  return 0;
}


/* Operations on remote files */
static pax_io_status_t
remote_reader (void *closure, void *data, idx_t size, idx_t *ret_size)
{
  tar_archive_t *tar = closure;
  ptrdiff_t s = rmt_read (tar->fd, data, size);
  *ret_size = s + (s < 0);
  return s < 0 ? pax_io_failure : s == 0 ? pax_io_eof : pax_io_success;
}

static pax_io_status_t
remote_writer (void *closure, void *data, idx_t size, idx_t *ret_size)
{
  tar_archive_t *tar = closure;
  idx_t s = rmt_write (tar->fd, data, size);
  *ret_size = s;
  return s == 0 ? pax_io_failure : pax_io_success;
}

static int
remote_seek (void *closure, off_t offset)
{
  tar_archive_t *tar = closure;
  off_t off = rmt_lseek (tar->fd, offset, SEEK_SET);
  if (off == -1)
    return pax_io_failure;
  return pax_io_success;
}

static int
remote_open (void *closure, int pax_mode)
{
  tar_archive_t *tar = closure;
  int mode = (pax_mode & PAXBUF_READ) ? O_RDONLY :
              O_RDWR | ((pax_mode & PAXBUF_CREAT) ? O_CREAT : 0);
  tar->fd = rmt_open (tar->filename, mode, 0, tar->rsh, tar->rmt);
  if (tar->fd == -1)
    return pax_io_failure;
  return pax_io_success;
}

static int
remote_close (void *closure, int mode)
{
  tar_archive_t *tar = closure;
  int fd = tar->fd;
  tar->fd = -1;
  return rmt_close (fd);
}


static int
tar_destroy (void *closure)
{
  tar_archive_t *tar = closure;
  free (tar->filename);
  free (tar);
  return 0;
}

static int
tar_wrapper (void *closure)
{
  return 1;
}

void
tar_archive_create (paxbuf_t *pbuf, const char *filename,
		    int remote, int mode, idx_t bfactor)
{
  tar_archive_t *tar;

  tar = xmalloc (sizeof (*tar));
  tar->filename = xstrdup (filename);
  tar->fd = -1;
  tar->bfactor = bfactor;
  tar->rsh = nullptr;
  tar->rmt = nullptr;
  paxbuf_create (pbuf, mode, tar, bfactor * BLOCKSIZE);
  if (remote)
    {
      paxbuf_set_io (*pbuf, remote_reader, remote_writer, remote_seek);
      paxbuf_set_term (*pbuf, remote_open, remote_close, tar_destroy);
    }
  else
    {
      paxbuf_set_io (*pbuf, local_reader, local_writer, local_seek);
      paxbuf_set_term (*pbuf, local_open, local_close, tar_destroy);
    }

  paxbuf_set_wrapper (*pbuf, tar_wrapper);
}

void
tar_set_rmt (paxbuf_t pbuf, const char *rmt)
{
  tar_archive_t *tar = paxbuf_get_data (pbuf);
  tar->rmt = rmt;
}

void
tar_set_rsh (paxbuf_t pbuf, const char *rsh)
{
  tar_archive_t *tar = paxbuf_get_data (pbuf);
  tar->rsh = rsh;
}
