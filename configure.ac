## autoconf configure.ac && autoreconf -if

################################################################################
# This file is free and unencumbered software released into the public domain.
#
# Anyone is free to copy, modify, publish, use, compile, sell, or distribute 
# this file, for any purpose, and by any means.
#
# In jurisdictions that recognize copyright laws, the author or authors of this 
# file dedicate any and all copyright interest in the file to the public
# domain. We make this dedication for the benefit of the public at large and to 
# the detriment of our heirs and successors. We intend this dedication to be an 
# overt act of relinquishment in perpetuity of all present and future rights to 
# this file under copyright law.
#
# THE FILE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
# AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN 
# ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION 
# WITH THIS FILE OR THE USE OR OTHER DEALINGS IN THIS FILE.
#
# For more information, please refer to http://unlicense.org/
################################################################################

AC_INIT(DESCRIPTION)

#UNAME=AC_CANONICAL_HOST
UNAME=`uname`
MAKE=make

PKG_LIBS=""

if test x$UNAME = xDarwin ; then
	EXTRA_LIBS=-framework CoreServices
fi

if test x$UNAME = xSunOS ; then
	EXTRA_LIBS=-lkstat -lsendfile
fi

if test x$UNAME = xFreeBSD ; then
	EXTRA_LIBS=-lkvm
	MAKE=gmake
fi

AC_SUBST(EXTRA_LIBS)
AC_SUBST(MAKE)

AC_OUTPUT(src/Makevars)
