#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.71])
AC_INIT([Wii U Env Selector], [0.0], [])
AC_CONFIG_SRCDIR([src/main.cpp])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])

AC_CANONICAL_HOST

DEVKITPRO_WUT_INIT

AM_INIT_AUTOMAKE([foreign subdir-objects tar-pax])

LT_INIT

AC_PROG_CC
AC_PROG_CXX
AX_APPEND_COMPILE_FLAGS([-std=c++23], [CXX])
AC_LANG([C++])

DEVKITPRO_WUT_SETUP

AX_PTHREAD([
                AX_APPEND_FLAG([$PTHREAD_CFLAGS], [CFLAGS])
                AX_APPEND_FLAG([$PTHREAD_CFLAGS], [CXXFLAGS])
                AX_APPEND_FLAG([$PTHREAD_LIBS],   [LIBS])
           ])

PKG_CHECK_MODULES([FT], [freetype2])

WIIU_ENV_SETUP_LIBMOCHA([external/libmocha])

AC_CONFIG_FILES([Makefile
                 external/Makefile])
AC_OUTPUT
