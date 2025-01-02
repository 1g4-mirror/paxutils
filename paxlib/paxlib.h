/* This file is part of GNU paxutils

   Copyright (C) 1994-2001, 2003, 2005, 2007, 2023-2025 Free Software
   Foundation, Inc.

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

#ifndef _paxlib_h_
#define _paxlib_h_

/* Error reporting functions and definitions */

/* Exit status for paxutils app.  Let's try to keep this list as simple as
   possible. tar -d option strongly invites a status different for unequal
   comparison and other errors.  */
enum { PAXEXIT_SUCCESS, PAXEXIT_DIFFERS, PAXEXIT_FAILURE };

/* The invoking can assign to this variable.  */
extern void (*error_hook) (void);

/* The invoking code must define this function.  It takes an exit
   status, outputs a usage message, then exits with that status.  */
_Noreturn void usage (int);

/* The invoking code can optionally define this variable.
   It defaults to a variable initialized to PAXEXIT_SUCCESS.  */
extern int exit_status;

/* Functions for issuing diagnostics.
   They all invoke 'error_hook' first, if it is a non-null function pointer.
   They accept an errno value, a printf format, and extra arguments
   to fill out the format, and output a diagnostic to stderr;
   a nonzero errno value also outputs the corresponding strerror string.
   paxwarn continues processing and does not affect the exit status.
   paxerror continues processing, but arranges for unsuccessful exit later.
   paxfatal exits unsuccessfully right away.
   paxusage is like paxfatal, except it does not take an errno value
   and suggests --help too.  */

void paxwarn (int, char const *, ...)
  _GL_ATTRIBUTE_COLD _GL_ATTRIBUTE_FORMAT ((printf, 2, 3));
void paxerror (int, char const *, ...)
  _GL_ATTRIBUTE_COLD _GL_ATTRIBUTE_FORMAT ((printf, 2, 3));
_Noreturn void paxfatal (int, char const *, ...)
  _GL_ATTRIBUTE_COLD _GL_ATTRIBUTE_FORMAT ((printf, 2, 3));
_Noreturn void paxusage (char const *, ...)
  _GL_ATTRIBUTE_COLD _GL_ATTRIBUTE_FORMAT ((printf, 1, 2));

/* Obsolete macros; callers should switch to paxwarn etc.  */
#define SHIFT1(arg1, ...) __VA_ARGS__
#define SHIFT2(arg1, arg2, ...) __VA_ARGS__
#define WARN(args) paxwarn (SHIFT1 args)
#define ERROR(args) paxerror (SHIFT1 args)
#define FATAL_ERROR(args) paxfatal (SHIFT1 args)
#define USAGE_ERROR(args) paxusage (SHIFT2 args)

void pax_decode_mode (mode_t mode, char *string);
void call_arg_error (char const *call, char const *name);
_Noreturn void call_arg_fatal (char const *call, char const *name);
void call_arg_warn (char const *call, char const *name);
void chmod_error_details (char const *name, mode_t mode);
void chown_error_details (char const *name, uid_t uid, gid_t gid);

void decode_mode (mode_t, char *);

_Noreturn void chdir_fatal (char const *);
void chmod_error_details (char const *, mode_t);
void chown_error_details (char const *, uid_t, gid_t);
void close_error (char const *);
void close_warn (char const *);
_Noreturn void exec_fatal (char const *);
void link_error (char const *, char const *);
void mkdir_error (char const *);
void mkfifo_error (char const *);
void mknod_error (char const *);
void open_error (char const *);
_Noreturn void open_fatal (char const *);
void open_warn (char const *);
void read_error (char const *);
void read_error_details (char const *, off_t, idx_t);
_Noreturn void read_fatal (char const *);
_Noreturn void read_fatal_details (char const *, off_t, idx_t);
void read_warn_details (char const *, off_t, idx_t);
void readlink_error (char const *);
void readlink_warn (char const *);
void rmdir_error (char const *);
void savedir_error (char const *);
void savedir_warn (char const *);
void seek_error (char const *);
void seek_error_details (char const *, off_t);
void seek_warn (char const *);
void seek_warn_details (char const *, off_t);
_Noreturn void stat_fatal (char const *);
void stat_error (char const *);
void stat_warn (char const *);
void symlink_error (char const *, char const *);
void truncate_error (char const *);
void truncate_warn (char const *);
void unlink_error (char const *);
void utime_error (char const *);
void waitpid_error (char const *);
void write_error (char const *);
void write_error_details (char const *, idx_t, idx_t);

_Noreturn void pax_exit (void);
_Noreturn void fatal_exit (void);

/* Name-related functions */
bool removed_prefixes_p (void);
char *safer_name_suffix (char const *file_name, bool link_target, bool absolute_names);

#endif
