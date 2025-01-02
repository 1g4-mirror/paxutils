/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2007, 2023-2025 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _ARGCV_H
#define _ARGCV_H 1

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int argcv_get (char const *command, char const *delim, char const* cmnt,
		      int *argc, char ***argv);
extern int argcv_string (int argc, char **argv, char **string);
extern int argcv_unescape_char (int c);
extern int argcv_escape_char (int c);

#ifdef __cplusplus
}
#endif

#endif /* _ARGCV_H */
