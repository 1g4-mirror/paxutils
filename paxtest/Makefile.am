# This file is part of GNU paxutils
#
# Copyright (C) 2005, 2007, 2009, 2023-2025 Free Software Foundation, Inc.
#
# Written by Sergey Poznyakoff
#
# GNU paxutils is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 3, or (at your option) any later
# version.
#
# GNU paxutils is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
# Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with GNU paxutils.  If not, see <http://www.gnu.org/licenses/>.

noinst_PROGRAMS = paxtest
paxtest_SOURCES = paxtest.c
noinst_HEADERS = paxtest.h

AM_CPPFLAGS = -I$(top_srcdir)/gnu -I../ -I../gnu -I../lib  -I../paxlib

LDADD = ../paxlib/libpax.a ../gnu/libgnu.a $(LIBINTL) $(LIBICONV)

