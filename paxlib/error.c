/* Miscellaneous error functions

   Copyright (C) 2005, 2007, 2023-2025 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 3, or (at your option) any later
   version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
   Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program.  If not, see <http://www.gnu.org/licenses/>. */

#include <system.h>
#include <paxlib.h>
#include <quote.h>
#include <quotearg.h>

void (*error_hook) (void);

void
paxwarn (int errnum, char const *format, ...)
{
  if (error_hook)
    error_hook ();
  va_list ap;
  va_start (ap, format);
  verror (0, errnum, format, ap);
  va_end (ap);
}

void
paxerror (int errnum, char const *format, ...)
{
  if (error_hook)
    error_hook ();
  va_list ap;
  va_start (ap, format);
  verror (0, errnum, format, ap);
  va_end (ap);
  exit_status = PAXEXIT_FAILURE;
}

void
paxfatal (int errnum, char const *format, ...)
{
  if (error_hook)
    error_hook ();
  va_list ap;
  va_start (ap, format);
  verror (0, errnum, format, ap);
  va_end (ap);
  fatal_exit ();
}

void
paxusage (char const *format, ...)
{
  if (error_hook)
    error_hook ();
  va_list ap;
  va_start (ap, format);
  verror (0, 0, format, ap);
  va_end (ap);
  usage (PAXEXIT_FAILURE);
}

/* Decode MODE from its binary form in a stat structure, and encode it
   into a 9-byte string STRING, terminated with a NUL.  */

void
pax_decode_mode (mode_t mode, char *string)
{
  *string++ = mode & S_IRUSR ? 'r' : '-';
  *string++ = mode & S_IWUSR ? 'w' : '-';
  *string++ = (mode & S_ISUID
	       ? (mode & S_IXUSR ? 's' : 'S')
	       : (mode & S_IXUSR ? 'x' : '-'));
  *string++ = mode & S_IRGRP ? 'r' : '-';
  *string++ = mode & S_IWGRP ? 'w' : '-';
  *string++ = (mode & S_ISGID
	       ? (mode & S_IXGRP ? 's' : 'S')
	       : (mode & S_IXGRP ? 'x' : '-'));
  *string++ = mode & S_IROTH ? 'r' : '-';
  *string++ = mode & S_IWOTH ? 'w' : '-';
  *string++ = (mode & S_ISVTX
	       ? (mode & S_IXOTH ? 't' : 'T')
	       : (mode & S_IXOTH ? 'x' : '-'));
  *string = '\0';
}

/* Report an error associated with the system call CALL and the
   optional name NAME.  */
void
call_arg_error (char const *call, char const *name)
{
  /* TRANSLATORS: %s after `Cannot' is a function name, e.g. `Cannot open'.
     Directly translating this to another language will not work, first because
     %s itself is not translated.
     Translate it as `%s: Function %s failed'. */
  paxerror (errno, _("%s: Cannot %s"), quotearg_colon (name), call);
}

/* Report a fatal error associated with the system call CALL and
   the optional file name NAME.  */
void
call_arg_fatal (char const *call, char const *name)
{
  /* TRANSLATORS: %s after `Cannot' is a function name, e.g. `Cannot open'.
     Directly translating this to another language will not work, first because
     %s itself is not translated.
     Translate it as `%s: Function %s failed'. */
  paxfatal (errno, _("%s: Cannot %s"), quotearg_colon (name),  call);
}

/* Report a warning associated with the system call CALL and
   the optional file name NAME.  */
void
call_arg_warn (char const *call, char const *name)
{
  /* TRANSLATORS: %s after `Cannot' is a function name, e.g. `Cannot open'.
     Directly translating this to another language will not work, first because
     %s itself is not translated.
     Translate it as `%s: Function %s failed'. */
  paxwarn (errno, _("%s: Warning: Cannot %s"), quotearg_colon (name), call);
}

void
chmod_error_details (char const *name, mode_t mode)
{
  char buf[10];
  pax_decode_mode (mode, buf);
  paxerror (errno, _("%s: Cannot change mode to %s"), quotearg_colon (name), buf);
}

void
chown_error_details (char const *name, uid_t uid, gid_t gid)
{
  uintmax_t u = uid, g = gid;
  paxerror (errno, _("%s: Cannot change ownership to uid %ju, gid %ju"),
	    quotearg_colon (name), u, g);
}

void
close_error (char const *name)
{
  call_arg_error ("close", name);
}

void
close_warn (char const *name)
{
  call_arg_warn ("close", name);
}

void
exec_fatal (char const *name)
{
  call_arg_fatal ("exec", name);
}

void
link_error (char const *target, char const *source)
{
  paxerror (errno, _("%s: Cannot hard link to %s"),
	    quotearg_colon (source), quote_n (1, target));
}

void
mkdir_error (char const *name)
{
  call_arg_error ("mkdir", name);
}

void
mkfifo_error (char const *name)
{
  call_arg_error ("mkfifo", name);
}

void
mknod_error (char const *name)
{
  call_arg_error ("mknod", name);
}

void
open_error (char const *name)
{
  call_arg_error ("open", name);
}

void
open_fatal (char const *name)
{
  call_arg_fatal ("open", name);
}

void
open_warn (char const *name)
{
  call_arg_warn ("open", name);
}

void
read_error (char const *name)
{
  call_arg_error ("read", name);
}

void
read_error_details (char const *name, off_t offset, idx_t size)
{
  intmax_t off = offset;
  paxerror (errno,
	    ngettext ("%s: Read error at byte %jd, while reading %td byte",
		      "%s: Read error at byte %jd, while reading %td bytes",
		      size),
	    quotearg_colon (name), off, size);
}

void
read_warn_details (char const *name, off_t offset, idx_t size)
{
  intmax_t off = offset;
  paxwarn (errno,
	   ngettext (("%s: Warning: Read error at byte %jd,"
		      " while reading %td byte"),
		     ("%s: Warning: Read error at byte %jd,"
		      " while reading %td bytes"),
		     size),
	   quotearg_colon (name), off, size);
}

void
read_fatal (char const *name)
{
  call_arg_fatal ("read", name);
}

void
read_fatal_details (char const *name, off_t offset, idx_t size)
{
  intmax_t off = offset;
  paxfatal (errno,
	    ngettext ("%s: Read error at byte %jd, while reading %td byte",
		      "%s: Read error at byte %jd, while reading %td bytes",
		      size),
	    quotearg_colon (name), off, size);
}

void
readlink_error (char const *name)
{
  call_arg_error ("readlink", name);
}

void
readlink_warn (char const *name)
{
  call_arg_warn ("readlink", name);
}

void
rmdir_error (char const *name)
{
  call_arg_error ("rmdir", name);
}

void
savedir_error (char const *name)
{
  call_arg_error ("savedir", name);
}

void
savedir_warn (char const *name)
{
  call_arg_warn ("savedir", name);
}

void
seek_error (char const *name)
{
  call_arg_error ("seek", name);
}

void
seek_error_details (char const *name, off_t offset)
{
  intmax_t off = offset;
  paxerror (errno, _("%s: Cannot seek to %jd"), quotearg_colon (name), off);
}

void
seek_warn (char const *name)
{
  call_arg_warn ("seek", name);
}

void
seek_warn_details (char const *name, off_t offset)
{
  intmax_t off = offset;
  paxwarn (errno, _("%s: Warning: Cannot seek to %jd"),
	   quotearg_colon (name), off);
}

void
symlink_error (char const *contents, char const *name)
{
  paxerror (errno, _("%s: Cannot create symlink to %s"),
	    quotearg_colon (name), quote_n (1, contents));
}

void
stat_fatal (char const *name)
{
  call_arg_fatal ("stat", name);
}

void
stat_error (char const *name)
{
  call_arg_error ("stat", name);
}

void
stat_warn (char const *name)
{
  call_arg_warn ("stat", name);
}

void
truncate_error (char const *name)
{
  call_arg_error ("truncate", name);
}

void
truncate_warn (char const *name)
{
  call_arg_warn ("truncate", name);
}

void
unlink_error (char const *name)
{
  call_arg_error ("unlink", name);
}

void
utime_error (char const *name)
{
  call_arg_error ("utime", name);
}

void
waitpid_error (char const *name)
{
  call_arg_error ("waitpid", name);
}

void
write_error (char const *name)
{
  call_arg_error ("write", name);
}

void
write_error_details (char const *name, idx_t status, idx_t size)
{
  if (status == 0)
    write_error (name);
  else
    paxerror (0,
	      ngettext ("%s: Wrote only %td of %td byte",
			"%s: Wrote only %td of %td bytes",
			size),
	      name, status, size);
}

void
chdir_fatal (char const *name)
{
  call_arg_fatal ("chdir", name);
}
