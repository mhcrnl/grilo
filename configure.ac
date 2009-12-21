#
# configure.ac
# 
# Author: Iago Toral Quiroga <itoral@igalia.com>
#
# Copyright (C) 2010 Igalia S.L.. All rights reserved.

m4_define([prj_name],    [media-store])
m4_define([prj_version], [0.0.1-0])

AC_INIT([prj_name], [prj_version])

AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)

AC_CONFIG_SRCDIR([src])

AM_CONFIG_HEADER(src/config.h)

AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL

AM_PROG_CC_C_O

AC_HEADER_STDC

AM_MAINTAINER_MODE

CFLAGS="$CFLAGS -Wall -Werror -Wmissing-prototypes -Wmissing-declarations -std=c99"

# ----------------------------------------------------------
# UNINSTALLED SUPPORT
# ----------------------------------------------------------

AC_ARG_ENABLE(uninstalled,
        [  --enable-uninstalled run uninstalled],,
        enable_uninstalled=no)
if test "x$enable_uninstalled" = "xyes"; then
        AC_MSG_WARN("Configuring for running uninstalled!")
fi

AM_CONDITIONAL([UNINSTALLED], [test "x$enable_uninstalled" = "xyes"])

# ----------------------------------------------------------
# DEBUG SUPPORT
# ----------------------------------------------------------

AC_ARG_ENABLE(uninstalled,
        [  --enable-debug include debug symbols],,
        enable_debug=no)
if test "x$enable_debug" = "xyes"; then
        AC_MSG_WARN("Configuring with debugging options enabled!")
	CFLAGS="$CFLAGS -g3 -O0"
fi

AM_CONDITIONAL([UNINSTALLED], [test "x$enable_uninstalled" = "xyes"])

# ----------------------------------------------------------
# GETTEXT
# ----------------------------------------------------------

GETTEXT_PACKAGE=media-store
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package])

AM_GLIB_GNU_GETTEXT

# ----------------------------------------------------------
# DEPENDENCIES
# ----------------------------------------------------------

PKG_CHECK_MODULES(DEPS, glib-2.0 \
			gobject-2.0 \
			gmodule-2.0)


AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)

PKG_CHECK_MODULES(VFS, gnome-vfs-2.0)

AC_SUBST(VFS_CFLAGS)
AC_SUBST(VFS_LIBS)

# ----------------------------------------------------------
# WORKAROUNDS
# ----------------------------------------------------------

AC_SUBST([abs_top_builddir])

# ----------------------------------------------------------
# OUTPUT
# ----------------------------------------------------------

AC_CONFIG_FILES([
  Makefile
  src/Makefile
  src/content/Makefile
  test/Makefile
])

AC_OUTPUT