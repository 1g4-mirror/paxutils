/* This file is part of GNU paxutils
   Copyright (C) 2005, 2007, 2010, 2023-2025 Free Software Foundation, Inc.

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
#include <hash.h>
#include <paxlib.h>


/* Hash tables of strings.  */

/* Calculate the hash of a string.  */
static size_t
hash_string_hasher (void const *name, size_t n_buckets)
{
  return hash_string (name, n_buckets);
}

/* Compare two strings for equality.  */
static bool
hash_string_compare (void const *name1, void const *name2)
{
  return strcmp (name1, name2) == 0;
}

/* Return zero if TABLE contains a LEN-character long prefix of STRING,
   otherwise, insert a newly allocated copy of this prefix to TABLE and
   return true.  If RETURN_PREFIX is nonnull, point it to the allocated
   copy. */
static bool
hash_string_insert_prefix (Hash_table **table, char const *string, size_t len,
			   const char **return_prefix)
{
  Hash_table *t = *table;
  char *s = ximemdup0 (string, len);
  char *e;

  if (! ((t
	  || (*table = t = hash_initialize (0, nullptr, hash_string_hasher,
					    hash_string_compare, nullptr)))
	 && (e = hash_insert (t, s))))
    xalloc_die ();

  if (e == s)
    {
      if (return_prefix)
	*return_prefix = s;
      return true;
    }
  else
    {
      free (s);
      return false;
    }
}


static Hash_table *prefix_table[2];

/* Return true if file names of some members in the archive were stripped off
   their leading components. We could have used
        return prefix_table[0] || prefix_table[1]
   but the following seems to be safer: */
bool
removed_prefixes_p (void)
{
  return (prefix_table[0] && hash_get_n_entries (prefix_table[0]) != 0)
         || (prefix_table[1] && hash_get_n_entries (prefix_table[1]) != 0);
}

/* Return a safer suffix of FILE_NAME, or "." if it has no safer suffix.
   Skip any sequence of prefixes each of which would cause
   the file name to escape the working directory on this platform.
   Warn the user if we do not return NAME.  If LINK_TARGET,
   FILE_NAME is the target of a hard link, not a member name.
   However, if ABSOLUTE_NAMES, do not skip prefixes, but instead
   return FILE_NAME if nonempty, "." otherwise.  */

char *
safer_name_suffix (char const *file_name, bool link_target,
		   bool absolute_names)
{
  char const *p = file_name;

  if (!absolute_names)
    {
      /* Skip any sequences of prefixes each of which would cause the
	 resulting file name to escape the working directory on this platform.
         The resulting file name is relative, not absolute.  */
      for (;;)
	{
	  if (ISSLASH (*p))
	    p++;
	  else if (p[0] == '.' && p[1] == '.' && (ISSLASH (p[2]) || !p[2]))
	    p += 2;
	  else
	    {
	      int prefix_len = FILE_SYSTEM_PREFIX_LEN (p);
	      if (prefix_len == 0)
		break;
	      p += prefix_len;
	    }
	}

      if (p != file_name)
	{
	  const char *prefix;
	  if (hash_string_insert_prefix (&prefix_table[link_target], file_name,
					 p - file_name, &prefix))
	    {
	      static char const *const diagnostic[] =
	      {
		N_("Removing leading `%s' from member names"),
		N_("Removing leading `%s' from hard link targets")
	      };
	      paxwarn (0, _(diagnostic[link_target]), prefix);
	    }
	}
    }

  if (! *p)
    {
      if (p == file_name)
	{
	  static char const *const diagnostic[] =
	  {
	    N_("Substituting `.' for empty member name"),
	    N_("Substituting `.' for empty hard link target")
	  };
	  paxwarn (0, "%s", _(diagnostic[link_target]));
	}

      p = ".";
    }

  return (char *) p;
}
