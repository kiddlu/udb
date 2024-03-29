# SPDX-License-Identifier: GPL-2.0-only
# Copyright (c) 2009 Kay Sievers <kay.sievers@vrfy.org>

AC_INIT([usbutils],
	[015],
	[linux-usb@vger.kernel.org])
AC_PREREQ(2.60)
AM_INIT_AUTOMAKE([check-news foreign 1.9 dist-bzip2])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AC_PREFIX_DEFAULT([/usr])

AC_USE_SYSTEM_EXTENSIONS
AC_SYS_LARGEFILE

AC_CHECK_HEADERS([byteswap.h])
AC_CHECK_FUNCS([nl_langinfo iconv])

PKG_CHECK_MODULES(LIBUSB, libusb-1.0 >= 1.0.14)

PKG_CHECK_MODULES(UDEV, libudev >= 196)

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
	Makefile
])
AC_CONFIG_SUBDIRS([usbhid-dump])

AC_OUTPUT
AC_MSG_RESULT([
	usbutils $VERSION
	=============

	prefix:                 ${prefix}
	datarootdir:            ${datarootdir}
	datadir:                ${datadir}
	mandir:                 ${mandir}

	usb.ids:                ${datadir}/usb.ids

	compiler:               ${CC}
	cflags:                 ${CFLAGS}
	ldflags:                ${LDFLAGS}
])
