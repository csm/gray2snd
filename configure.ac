dnl Process this file with autoconf to produce configure.
dnl
dnl vim:set softtabstop=4 shiftwidth=4 tabstop=4 expandtab tw=72:
dnl
dnl Copyright (C) 2003  Casey Marshall <rsdio@metastatic.org>
dnl
dnl This program is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 2 of the License, or
dnl (at your option) any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the
dnl
dnl    Free Software Foundation, Inc.,
dnl    59 Temple Place, Suite 330,
dnl    Boston, MA  02111-1307
dnl    USA
dnl
dnl --------------------------------------------------------------------------

AC_INIT(gray2snd, 0.3, rsdio@metastatic.org)
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(config.h)
AM_MAINTAINER_MODE

dnl --------------------------------------------------------------------------
dnl Checks for programs.
dnl --------------------------------------------------------------------------

AC_PROG_CC

dnl --------------------------------------------------------------------------
dnl Checks for libraries.
dnl --------------------------------------------------------------------------

AC_CHECK_LIB(sndfile, sf_open,, AC_MSG_ERROR([libsndfile not found]))
AC_CHECK_LIB(popt, poptGetContext,, AC_MSG_ERROR([popt library not found]))
AC_CHECK_LIB(m, sin,, AC_MSG_ERROR([NO LIBM OMG WTF!?!]))

have_fftw=no
AC_CHECK_LIB(rfftw, rfftw_one, have_fftw=yes, , [-lfftw -lm])
if test "$have_fftw" = "yes"; then
    LIBS="$LIBS -lrfftw -lfftw"
else
    AC_CHECK_LIB(drfftw, rfftw_one, have_fftw=yes,
        AC_MSG_ERROR([FFTW library not found.]), [-ldfftw -lm])
    LIBS="$LIBS -ldrfftw -ldfftw"
    AC_DEFINE(HAVE_FFTW_PREFIX, 1, [Define if you use `dfftw'.])
fi

AC_OUTPUT([Makefile])
