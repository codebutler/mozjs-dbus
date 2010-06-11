#
# Makefile.am
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

# FIXME: Move these into configure.ac
XPIDL = /usr/lib/xulrunner-1.9.2.3/xpidl
IDL_INCLUDES = -I /usr/share/idl/xulrunner-1.9.2.3/


DEST = $(top_builddir)/xpi/components

COMPONENT = $(DEST)/MozJSDBus

IDL_NAME = IMozJSDBusCoreComponent

SOURCES = MozJSDBusCoreComponent.cpp \
          MozJSDBusCoreComponentModule.cpp \
          MozJSDBusObjectPath.cpp \
          MozJSDBusMarshalling.cpp

CPPFLAGS = -shared -fno-rtti -fPIC

all: $(NAME).h $(NAME).xpt $(COMPONENT).so

$(NAME).h:
	$(XPIDL) -m header $(IDL_INCLUDES) $(IDL_NAME).idl

$(NAME).xpt:
	mkdir -p $(DEST)
	$(XPIDL) -m typelib $(IDL_INCLUDES) -e $(DEST)/$(IDL_NAME).xpt $(IDL_NAME).idl	

$(COMPONENT).so: $(SOURCES)
	mkdir -p $(DEST)
	$(CXX) $(SOURCES) -g -Wall -o $@ $(XUL_CFLAGS) $(DBUS_CFLAGS) $(XUL_LIBS) $(DBUS_LIBS) $(CPPFLAGS) $(CXXFLAGS) 

clean:
	rm -f $(NAME).h $(DEST)/$(IDL_NAME).xpt $(COMPONENT).so
