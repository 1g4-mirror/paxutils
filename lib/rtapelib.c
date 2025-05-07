/* Functions for communicating with a remote tape drive.

   Copyright 1988-2025 Free Software Foundation, Inc.

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

/* The man page rmt(8) for /etc/rmt documents the remote mag tape protocol
   which rdump and rrestore use.  Unfortunately, the man page is *WRONG*.
   The author of the routines I'm including originally wrote his code just
   based on the man page, and it didn't work, so he went to the rdump source
   to figure out why.  The only thing he had to change was to check for the
   'F' return code in addition to the 'E', and to separate the various
   arguments with \n instead of a space.  I personally don't think that this
   is much of a problem, but I wanted to point it out. -- Arnold Robbins

   Originally written by Jeff Lee, modified some by Arnold Robbins.  Redone
   as a library that can replace open, read, write, etc., by Fred Fish, with
   some additional work by Arnold Robbins.  Modified to make all rmt* calls
   into macros for speed by Jay Fenlason.  Use -DWITH_REXEC for rexec
   code, courtesy of Dan Kegel.  */

#include "system.h"

#include <verify.h>

#include <signal.h>

#if HAVE_SYS_MTIO_H
# include <sys/mtio.h>
#endif

#if HAVE_NETDB_H
# include <netdb.h>
#endif

/* Exit status if exec errors.  */
enum { EXIT_ON_EXEC_ERROR = 128 };

/* FIXME: Size of buffers for reading and writing commands to rmt.  */
enum { COMMAND_BUFFER_SIZE = 64 };

/* FIXME: Maximum number of simultaneous remote tape connections.  */
enum { MAXUNIT = 4 };

/* Read and write file descriptors from pipe () calls.  */
enum { PREAD, PWRITE };		/* read  file descriptor from pipe() */

/* The pipes for receiving data from remote tape drives.  */
static int from_remote[MAXUNIT][2] = {{-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}};

/* The pipes for sending data to remote tape drives.  */
static int to_remote[MAXUNIT][2] = {{-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}};

/* The parent's read side of remote tape connection Fd.  */
static int
read_side (int handle)
{
  return from_remote[handle][PREAD];
}

/* The parent's write side of remote tape connection Fd.  */
static int
write_side (int handle)
{
  return to_remote[handle][PWRITE];
}

/* ../paxlib/rtape.c #defines PAXLIB_RTAPE.  */
#ifdef PAXLIB_RTAPE
# include <localedir.h> /* for DEFAULT_RMT_COMMAND */
#else
# define RMT_INLINE _GL_EXTERN_INLINE
# include <rmt.h>
# include <rmt-command.h>

char const *rmt_command = DEFAULT_RMT_COMMAND;

/* If true, always consider file names to be local, even if they contain
   colons */
bool force_local_option;

# define _rmt_rexec (host, user, rmt_command) _rmt_rexec (host, user)
# define rmt_open(file_name, oflags, bias, remote_shell, rmt_command) \
    rmt_open__ (file_name, oflags, bias, remote_shell)
# define rmt_close(handle) rmt_close__ (handle)
# define rmt_read(handle, buffer, length) rmt_read__ (handle, buffer, length)
# define rmt_write(handle, buffer, length) rmt_write__ (handle, buffer, length)
# define rmt_lseek(handle, offset, whence) rmt_lseek__ (handle, offset, whence)
# define rmt_ioctl(handle, operation, argument) \
    rmt_ioctl__ (handle, operation, argument)
#endif



/* Close remote tape connection HANDLE, and reset errno to ERRNO_VALUE.  */
static void
_rmt_shutdown (int handle, int errno_value)
{
  close (read_side (handle));
  close (write_side (handle));
  from_remote[handle][PREAD] = -1;
  to_remote[handle][PWRITE] = -1;
  errno = errno_value;
}

/* Using remote tape connection HANDLE, do the remote tape command
   specified in BUFFER with LENGTH bytes.
   Return 0 if successful, -1 on error.  */
static int
do_command (int handle, const char *buffer, idx_t length)
{
  /* Save the current pipe handler and try to make the request.  */

  void (*pipe_handler) (int) = signal (SIGPIPE, SIG_IGN);
  idx_t written = full_write (write_side (handle), buffer, length);
  signal (SIGPIPE, pipe_handler);

  if (written == length)
    return 0;

  /* Something went wrong.  Close down and go home.  */

  _rmt_shutdown (handle, EIO);
  return -1;
}

/* Return the integer represented by the array of bytes at the start of S.
   The bytes use the usual unsigned ASCII decimal representation,
   and are terminated by a non ASCII digit.  Return -1 if S does not
   start with a decimal string, or if the integer exceeds SMAX.  */
static intmax_t
dectointmax (char const *s, intmax_t smax)
{
  /* Do not use strtoimax, as it allows leading whitespace, '-' and
     '+', and requires the input to be null-terminated.  */
  if ('0' <= *s && *s <= '9')
    {
      bool overflow = false;
      intmax_t result = *s - '0';
      while ('0' <= *++s && *s <= '9')
	{
	  overflow |= ckd_mul (&result, result, 10);
	  overflow |= ckd_add (&result, result, *s - '0');
	}
      overflow |= smax < result;
      if (!overflow)
	{
	  assume (0 <= result);
	  return result;
	}
    }
  return -1;
}

/* Get from HANDLE a response string into COMMAND_BUFFER, terminated by '\n'.
   If the response is a successful command, starting with 'A',
   return the address of the byte following the 'A'.
   Otherwise, set errno and return a null pointer.  */
static char *
get_status_string (int handle, char command_buffer[COMMAND_BUFFER_SIZE])
{
  char *cursor;

  /* Read the reply command line.  */

  for (cursor = command_buffer; ; cursor++)
    {
      if (cursor == command_buffer + COMMAND_BUFFER_SIZE
	  || safe_read (read_side (handle), cursor, 1) != 1)
	{
	  _rmt_shutdown (handle, EIO);
	  return nullptr;
	}
      if (*cursor == '\n')
	break;
    }

  /* Check the return status.  */

  for (cursor = command_buffer; *cursor == ' '; cursor++)
    continue;

  if (*cursor == 'E' || *cursor == 'F')
    {
      /* Skip the error message line.  */

      /* FIXME: there is better to do than merely ignoring error messages
	 coming from the remote end.  Translate them, too...  */

      char character;
      do
	{
	  if (safe_read (read_side (handle), &character, 1) != 1)
	    {
	      _rmt_shutdown (handle, EIO);
	      return nullptr;
	    }
	}
      while (character == '\n');

      /* This assumes remote errno values are the same as local,
	 which is wrong in general, but does work in common cases
	 and perhaps is the best we can do here.  */
      int err = dectointmax (cursor + 1, INT_MAX);
      errno = err <= 0 ? EIO : err;

      if (*cursor == 'F')
	_rmt_shutdown (handle, errno);

      return nullptr;
    }

  /* Check for mis-synced pipes.  */

  if (*cursor != 'A')
    {
      _rmt_shutdown (handle, EIO);
      return nullptr;
    }

  /* Got an 'A' (success) response.  */

  return cursor + 1;
}

/* Read and return the status from remote tape connection HANDLE.
   The status must be in the range 0..STATUS_MAX.  If
   an error occurred, return -1 and set errno.  */
static intmax_t
get_status (int handle, intmax_t status_max)
{
  char command_buffer[COMMAND_BUFFER_SIZE];
  const char *status = get_status_string (handle, command_buffer);
  if (status)
    {
      intmax_t result = dectointmax (status, status_max);
      if (0 <= result)
	return result;
      errno = EIO;
    }
  return -1;
}

#if WITH_REXEC

/* Execute /etc/rmt as user USER on remote system HOST using rexec.
   Return a file descriptor of a bidirectional socket for stdin and
   stdout.  If USER is zero, use the current username.

   By default, this code is not used, since it requires that the user
   have a .netrc file in his/her home directory, or that the
   application designer be willing to have rexec prompt for login and
   password info.  This may be unacceptable, and .rhosts files for use
   with rsh are much more common on BSD systems.  */
static int
_rmt_rexec (char *host, char const *user, char const *rmt_command)
{
  struct servent *rexecserv = getservbyname ("exec", "tcp");
  if (!rexecserv)
    error (EXIT_ON_EXEC_ERROR, 0, _("exec/tcp: Service not available"));

  int saved_stdin = dup (STDIN_FILENO);
  if ((saved_stdin < 0 && errno != EBADF)
      || (saved_stdin == STDOUT_FILENO
	  && ((saved_stdin = dup (STDOUT_FILENO)) < 0
	      || close (STDOUT_FILENO) < 0)))
    error (EXIT_ON_EXEC_ERROR, errno, _("stdin"));
  int saved_stdout = dup (STDOUT_FILENO);
  if ((saved_stdout < 0 && errno != EBADF)
      || (saved_stdout == STDIN_FILENO
	  && ((saved_stdout = dup (STDIN_FILENO)) < 0
	      || close (STDIN_FILENO) < 0)))
    error (EXIT_ON_EXEC_ERROR, errno, _("stdout"));

  /* When using cpio -o < filename, stdin is no longer the tty.  But the
     rexec subroutine reads the login and the passwd on stdin, to allow
     remote execution of the command.  So, reopen stdin and stdout on
     /dev/tty before the rexec and give them back their original value
     after.  */

  if ((0 <= saved_stdin && close (STDIN_FILENO) < 0)
      || (open ("/dev/tty", O_RDONLY) < 0 && open ("/dev/null", O_RDONLY) < 0)
      || (0 <= saved_stdout && close (STDOUT_FILENO) < 0)
      || (open ("/dev/tty", O_WRONLY) < 0 && open ("/dev/null", O_WRONLY) < 0))
    error (EXIT_ON_EXEC_ERROR, errno, "/dev/null");

  int result = rexec (&host, rexecserv->s_port,
		      user, nullptr, rmt_command, nullptr);
  if (0 <= saved_stdin
      && (dup2 (saved_stdin, STDIN_FILENO) < 0 || close (saved_stdin) < 0))
    error (0, errno, _("stdin"));
  if (0 <= saved_stdout
      && (dup2 (saved_stdout, STDOUT_FILENO) < 0 || close (saved_stdout) < 0))
    error (0, errno, _("stdout"));

  return result;
}

#else

/* GCC 13 misunderstands the dup2 trickery below.  */
# if 13 <= __GNUC__
#  pragma GCC diagnostic ignored "-Wanalyzer-fd-leak"
# endif

#endif /* WITH_REXEC */

/* Place into BUF a string representing OFLAGS, which must be suitable
   as argument 2 of 'open'.  BUF must be large enough to hold the
   result.  This function should generate a string that decode_oflags
   can parse.  Return a pointer to the trailing null byte of the string
   if successful, a null pointer otherwise.  */
static char *
encode_oflags (char *buf, int oflags)
{
  char *p = buf + sprintf (buf, "%d ", oflags);

  switch (oflags & O_ACCMODE)
    {
    case O_RDONLY: p = stpcpy (p, "O_RDONLY"); break;
    case O_RDWR: p = stpcpy (p, "O_RDWR"); break;
    case O_WRONLY: p = stpcpy (p, "O_WRONLY"); break;
    default: return nullptr;
    }

#ifdef O_APPEND
  if (oflags & O_APPEND) p = stpcpy (p, "|O_APPEND");
#endif
  if (oflags & O_CREAT) p = stpcpy (p, "|O_CREAT");
#ifdef O_DSYNC
  if (oflags & O_DSYNC) p = stpcpy (p, "|O_DSYNC");
#endif
  if (oflags & O_EXCL) p = stpcpy (p, "|O_EXCL");
#ifdef O_LARGEFILE
  if (oflags & O_LARGEFILE) p = stpcpy (p, "|O_LARGEFILE");
#endif
#ifdef O_NOCTTY
  if (oflags & O_NOCTTY) p = stpcpy (p, "|O_NOCTTY");
#endif
  if (oflags & O_NONBLOCK) p = stpcpy (p, "|O_NONBLOCK");
#ifdef O_RSYNC
  if (oflags & O_RSYNC) p = stpcpy (p, "|O_RSYNC");
#endif
#ifdef O_SYNC
  if (oflags & O_SYNC) p = stpcpy (p, "|O_SYNC");
#endif
  if (oflags & O_TRUNC) p = stpcpy (p, "|O_TRUNC");
  return p;
}

/* Reset user and group IDs to be those of the real user.
   Return null on success, a failing syscall name (setting errno) on error.  */
static char const *
sys_reset_uid_gid (void)
{
#if !MSDOS
  uid_t uid = getuid ();
  gid_t gid = getgid ();
  struct passwd *pw = getpwuid (uid);

  if (!pw)
    return "getpwuid";
  if (initgroups (pw->pw_name, gid) < 0 && errno != EPERM)
    return "initgroups";
  if (gid != getegid () && setgid (gid) < 0 && errno != EPERM)
    return "setgid";
  if (uid != geteuid () && setuid (uid) < 0 && errno != EPERM)
    return "setuid";
#endif

  return nullptr;
}

/* Open a file (a magnetic tape device?) on the system specified in
   FILE_NAME, as the given user. FILE_NAME has the form '[USER@]HOST:FILE'.
   OFLAGS is O_RDONLY, O_WRONLY, etc.  If successful, return the
   remote pipe number plus BIAS.  REMOTE_SHELL may be overridden.  On
   error, return -1.  */
int
rmt_open (char const *file_name, int oflags, int bias,
	  char const *remote_shell, char const *rmt_command)
{
  int remote_pipe_number;	/* pseudo, biased file descriptor */
  char *file_name_copy;		/* copy of file_name string */
  char *remote_host;		/* remote host name */
  char *remote_file;		/* remote file name (often a device) */
  char *remote_user;		/* remote user name */

  /* Find an unused pair of file descriptors.  */

  for (remote_pipe_number = 0;
       remote_pipe_number < MAXUNIT;
       remote_pipe_number++)
    if (read_side (remote_pipe_number) < 0
	&& write_side (remote_pipe_number) < 0)
      break;

  if (remote_pipe_number == MAXUNIT)
    {
      errno = EMFILE;
      return -1;
    }

  /* Pull apart the system and device, and optional user.  */

  {
    char *cursor;

    file_name_copy = xstrdup (file_name);
    remote_host = file_name_copy;
    remote_user = nullptr;
    remote_file = nullptr;

    for (cursor = file_name_copy; *cursor; cursor++)
      switch (*cursor)
	{
	default:
	  break;

	case '\n':
	  /* Do not allow newlines in the file_name, since the protocol
	     uses newline delimiters.  */
	  free (file_name_copy);
	  errno = ENOENT;
	  return -1;

	case '@':
	  if (!remote_user)
	    {
	      remote_user = remote_host;
	      *cursor = '\0';
	      remote_host = cursor + 1;
	    }
	  break;

	case ':':
	  if (!remote_file)
	    {
	      *cursor = '\0';
	      remote_file = cursor + 1;
	    }
	  break;
	}
  }

  assume (remote_file);

  /* FIXME: Should somewhat validate the decoding, here.  */

  if (remote_user && *remote_user == '\0')
    remote_user = nullptr;

#if WITH_REXEC

  /* Execute the remote command using rexec.  */

  int fd = _rmt_rexec (remote_host, remote_user, rmt_command);
  if (fd < 0)
    {
      free (file_name_copy);
      return -1;
    }

  from_remote[remote_pipe_number][PREAD] = fd;
  to_remote[remote_pipe_number][PWRITE] = fd;

#else /* not WITH_REXEC */
  {
    const char *remote_shell_basename;
    pid_t status;

    /* Identify the remote command to be executed.  */

    if (!remote_shell)
      {
#ifdef REMOTE_SHELL
	remote_shell = REMOTE_SHELL;
#else
	free (file_name_copy);
	errno = EIO;
	return -1;
#endif
      }
    remote_shell_basename = last_component (remote_shell);

    /* Set up the pipes for the 'rsh' command, and fork.  */

    if (pipe (to_remote[remote_pipe_number]) < 0)
      {
	free (file_name_copy);
	return -1;
      }

    if (pipe (from_remote[remote_pipe_number]) < 0)
      {
	int e = errno;
	close (to_remote[remote_pipe_number][PREAD]);
	close (to_remote[remote_pipe_number][PWRITE]);
	free (file_name_copy);
	errno = e;
	return -1;
      }

    status = fork ();
    if (status < 0)
      {
	int e = errno;
	close (from_remote[remote_pipe_number][PREAD]);
	close (from_remote[remote_pipe_number][PWRITE]);
	close (to_remote[remote_pipe_number][PREAD]);
	close (to_remote[remote_pipe_number][PWRITE]);
	free (file_name_copy);
	errno = e;
	return -1;
      }

    if (status == 0)
      {
	/* Child.  */

	if (dup2 (to_remote[remote_pipe_number][PREAD], STDIN_FILENO) < 0
	    || (to_remote[remote_pipe_number][PREAD] != STDIN_FILENO
		&& close (to_remote[remote_pipe_number][PREAD]) < 0)
	    || (to_remote[remote_pipe_number][PWRITE] != STDIN_FILENO
		&& close (to_remote[remote_pipe_number][PWRITE]) < 0)
	    || dup2 (from_remote[remote_pipe_number][PWRITE], STDOUT_FILENO) < 0
	    || close (from_remote[remote_pipe_number][PREAD]) < 0
	    || close (from_remote[remote_pipe_number][PWRITE]) < 0)
	  error (EXIT_ON_EXEC_ERROR, errno,
		 _("Cannot redirect files for remote shell"));

	char const *reseterr = sys_reset_uid_gid ();
	if (reseterr)
	  error (EXIT_ON_EXEC_ERROR, errno,
		 _("Cannot reset uid and gid: %s"), reseterr);

	char const *cmd = rmt_command ? rmt_command : DEFAULT_RMT_COMMAND;

	if (remote_user)
	  execl (remote_shell, remote_shell_basename, remote_host,
		 "-l", remote_user, cmd, nullptr);
	else
	  execl (remote_shell, remote_shell_basename, remote_host,
		 cmd, nullptr);

	/* Bad problems if we get here.  */

	/* In a previous version, _exit was used here instead of exit.  */
	error (EXIT_ON_EXEC_ERROR, errno, _("Cannot execute remote shell"));
      }

    /* Parent.  */

    close (from_remote[remote_pipe_number][PWRITE]);
    close (to_remote[remote_pipe_number][PREAD]);
  }
#endif /* not WITH_REXEC */

  /* Attempt to open the tape device.  */

  {
    idx_t remote_file_len = strlen (remote_file);
    char *command_buffer = ximalloc (remote_file_len + 1000);
    char *p = command_buffer;
    *p++ = 'O';
    p = mempcpy (p, remote_file, remote_file_len);
    *p++ = '\n';
    p = encode_oflags (p, oflags);
    if (!p)
      {
	free (command_buffer);
	free (file_name_copy);
	_rmt_shutdown (remote_pipe_number, EINVAL);
	return -1;
      }
    *p++ = '\n';
    int done = do_command (remote_pipe_number,
			   command_buffer, p - command_buffer);
    free (command_buffer);
    if (done < 0 || get_status (remote_pipe_number, INTMAX_MAX) < 0)
      {
	free (file_name_copy);
	_rmt_shutdown (remote_pipe_number, errno);
	return -1;
      }
  }

  free (file_name_copy);
  return remote_pipe_number + bias;
}

/* Close remote tape connection HANDLE and shut down.  Return 0 if
   successful, -1 on error.  */
int
rmt_close (int handle)
{
  int done = do_command (handle, "C\n", 2);
  if (done < 0)
    return done;

  int status = get_status (handle, 0);
  _rmt_shutdown (handle, errno);
  return status;
}

/* Read from remote tape connection HANDLE into BUFFER, of size LENGTH.
   Return the number of bytes read on success, -1 (setting errno) on error.  */
ptrdiff_t
rmt_read (int handle, void *buffer, idx_t length)
{
  char command_buffer[sizeof "R\n" + INT_STRLEN_BOUND (idx_t)];
  int done = do_command (handle, command_buffer,
			 sprintf (command_buffer, "R%jd\n", length));
  if (done < 0)
    return done;

  ptrdiff_t status = get_status (handle, length);
  if (status < 0)
    {
      _rmt_shutdown (handle, EIO);
      return -1;
    }

  char *buf = buffer;
  for (idx_t counter = 0; counter < status; )
    {
      ptrdiff_t rlen = safe_read (read_side (handle),
				  buf + counter, status - counter);
      if (rlen <= 0)
	{
	  _rmt_shutdown (handle, EIO);
	  return -1;
	}
      counter += rlen;
    }

  return status;
}

/* Write LENGTH bytes from BUFFER to remote tape connection HANDLE.
   Return the number of bytes written.  */
idx_t
rmt_write (int handle, void const *buffer, idx_t length)
{
  char command_buffer[sizeof "W\n" + INT_STRLEN_BOUND (idx_t)];
  void (*pipe_handler) (int);
  int buflen = sprintf (command_buffer, "W%jd\n", length);
  if (do_command (handle, command_buffer, buflen) < 0)
    return 0;

  pipe_handler = signal (SIGPIPE, SIG_IGN);
  idx_t written = full_write (write_side (handle), buffer, length);
  signal (SIGPIPE, pipe_handler);
  if (written == length)
    {
      ptrdiff_t r = get_status (handle, length);
      if (r < 0)
	return 0;
      if (r == length)
	return length;
      written = r;
    }

  /* Write error.  */

  _rmt_shutdown (handle, EIO);
  return written;
}

/* Perform an imitation lseek operation on remote tape connection
   HANDLE.  Return the new file offset if successful, -1 if on error.  */
off_t
rmt_lseek (int handle, off_t offset, int whence)
{
  char command_buffer[sizeof "L0\n\n" + INT_STRLEN_BOUND (offset)];
  intmax_t off = offset;

  switch (whence)
    {
    case SEEK_SET: whence = 0; break;
    case SEEK_CUR: whence = 1; break;
    case SEEK_END: whence = 2; break;
    default: errno = EINVAL; return -1;
    }

  int done = do_command (handle, command_buffer,
			 sprintf (command_buffer, "L%d\n%jd\n", whence, off));
  if (done < 0)
    return done;

  return get_status (handle, TYPE_MAXIMUM (off_t));
}

/* Perform a raw tape operation on remote tape connection HANDLE.
   Return the results of the ioctl, or -1 on error.  */
int
rmt_ioctl (int handle, unsigned long int operation, void *argument)
{
  switch (operation)
    {
    default:
      errno = EOPNOTSUPP;
      return -1;

#ifdef MTIOCTOP
    case MTIOCTOP:
      {
	struct mtop *mtop = argument;
	enum { oplen = INT_STRLEN_BOUND (mtop->mt_op) };
	enum { countlen = INT_STRLEN_BOUND (mtop->mt_count) };
	char command_buffer[sizeof "I\n\n" + oplen + countlen];

	/* MTIOCTOP is the easy one.  Nothing is transferred in binary.  */

	intmax_t count = mtop->mt_count;
	int done = do_command (handle, command_buffer,
			       sprintf (command_buffer, "I%d\n%jd\n",
					mtop->mt_op, count));
	if (done < 0)
	  return done;

	return get_status (handle, INT_MAX);
      }
#endif /* MTIOCTOP */

#ifdef MTIOCGET
    case MTIOCGET:
      {
	struct mtget *mtget = argument;

	/* Grab the status and read it directly into the structure.  This
	   assumes that the status buffer is not padded and that 2 shorts
	   fit in a long without any word alignment problems; i.e., the
	   whole struct is contiguous.  NOTE - this is probably NOT a good
	   assumption.  */

	int done = do_command (handle, "S\n", 2);
	if (done < 0)
	  return done;
	ptrdiff_t status = get_status (handle, sizeof *mtget);
	if (status < 0)
	  return status;
	if (status != sizeof *mtget)
	  {
	    _rmt_shutdown (handle, EIO);
	    return -1;
	  }

	for (char *p = argument; status > 0; )
	  {
	    ptrdiff_t rlen = safe_read (read_side (handle), p, status);
	    if (rlen <= 0)
	      {
		_rmt_shutdown (handle, EIO);
		return -1;
	      }
	    status -= rlen;
	    p += rlen;
	  }

	/* Check for byte position.  mt_type (or mt_model) is a small integer
	   field (normally) so we will check its magnitude.  If it is larger
	   than 256, we will assume that the bytes are swapped and go through
	   and reverse all the bytes.  */

	if (mtget->MTIO_CHECK_FIELD < 256)
	  return 0;

	char *buf = argument;
	static_assert (sizeof *mtget % 2 == 0);
	for (idx_t counter = 0; counter < sizeof *mtget; counter += 2)
	  {
	    char copy = buf[counter];

	    buf[counter] = buf[counter + 1];
	    buf[counter + 1] = copy;
	  }

	return 0;
      }
#endif /* MTIOCGET */

    }
}
