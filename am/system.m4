# Configuration for paxutils/lib/system.h.

# Copyright (C) 2005-2025 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

AC_DEFUN([PU_SYSTEM],[
  AC_CHECK_HEADERS_ONCE([grp.h pwd.h sys/mtio.h])

  AC_CHECK_MEMBERS([struct stat.st_blksize])
  AC_REQUIRE([AC_STRUCT_ST_BLOCKS])

  AC_CHECK_FUNCS_ONCE([mkfifo])
])
