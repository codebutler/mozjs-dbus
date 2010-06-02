#
# configure.ac
#
# Authors:
#   Eric Butler <eric@codebutler.com>
#
#  This file is part of mozjs-dbus.
#
#  mozjs-dbus is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  mozjs-dbus is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with mozjs-dbus.  If not, see <http://www.gnu.org/licenses/>.

AC_INIT([Helloworld C++], [0.5], [eric@codebutler.com],
        [mozjs_dbus], [http://example.com])
AC_PREREQ([2.59])
AM_INIT_AUTOMAKE([1.10 -Wall no-define])

PKG_CHECK_MODULES(XUL, [libxul >= 1.9.2.3, libxul-unstable >= 1.9.2.3])
AC_SUBST(XUL_CFLAGS)
AC_SUBST(XUL_LIBS)

PKG_CHECK_MODULES(DBUS, [dbus-1 >= 1.2.16, dbus-glib-1 >= 0.84])
AC_SUBST(DBUS_CFLAGS)
AC_SUBST(DBUS_LIBS)

AC_PROG_CXX

AC_CONFIG_FILES([
Makefile
src/Makefile
])
AC_OUTPUT

