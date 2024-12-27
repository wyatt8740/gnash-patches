dnl  
dnl  Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010,
dnl  2011, 2012 Free Software Foundation, Inc.
dnl  
dnl  This program is free software; you can redistribute it and/or modify
dnl  it under the terms of the GNU General Public License as published by
dnl  the Free Software Foundation; either version 3 of the License, or
dnl  (at your option) any later version.
dnl  
dnl  This program is distributed in the hope that it will be useful,
dnl  but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl  GNU General Public License for more details.
dnl  You should have received a copy of the GNU General Public License
dnl  along with this program; if not, write to the Free Software
dnl  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA


AC_DEFUN([GNASH_PATH_X11],
[

  has_x11=no

  dnl Look for the header
  AC_ARG_WITH(x11_incl, AS_HELP_STRING([--with-x11-incl], [Directory where x11 header is]), with_x11_incl=${withval})
  AC_CACHE_VAL(ac_cv_path_x11_incl, [
    if test x"${with_x11_incl}" != x ; then
      if test -f ${with_x11_incl}/X11/X.h ; then
       ac_cv_path_x11_incl="-I`(cd ${with_x11_incl}; pwd)`"
      else
       AC_MSG_ERROR([${with_x11_incl} directory doesn't contain X.h])
      fi
    fi
  ])

  dnl If the path hasn't been specified, go look for it.
  if test x"${ac_cv_path_x11_incl}" = x ; then
    newlist="/Developer/SDKs/MacOSX10.6*.sdk/usr/include /Developer/SDKs/MacOSX10.5*.sdk/usr/include /Developer/SDKs/MacOSX10.4*.sdk/usr/include ${incllist}"
    for i in $newlist; do
    	if test -f $i/X11/X.h; then
  	    ac_cv_path_x11_incl="-I$i"
        AC_DEFINE(HAVE_X11_X_H, [1], [Have the X11 X.h header file])
        break
    	fi
    done
  else
    AC_DEFINE(HAVE_X11_X_H, [1], [Have the X11 X.h header file])
  fi

  AC_MSG_CHECKING([for X11 headers])
  if test x"${ac_cv_path_x11_incl}" != x ; then
    dnl Don't pass -I/usr/include
    if test x"${ac_cv_path_x11_incl}" != "x-I/usr/include"; then
       X11_CFLAGS="${ac_cv_path_x11_incl}"
    fi
    x11_incl=yes
    AC_MSG_RESULT(${ac_cv_path_x11_incl})
  else
    x11_incl=""
    X11_CFLAGS=""
    AC_MSG_RESULT(none)
  fi

  dnl Look for the library
  AC_ARG_WITH(x11_lib, AS_HELP_STRING([--with-x11-lib], [directory where x11 library is]), with_x11_lib=${withval})
  AC_CACHE_VAL(ac_cv_path_x11_lib,[
    if test x"${with_x11_lib}" != x ; then
      if test -f ${with_x11_lib}/libX11.a -o -f ${with_x11_lib}/libX11.${shlibext}; then
       ac_cv_path_x11_lib="-L`(cd ${with_x11_lib}; pwd)`"
      else
       AC_MSG_ERROR([${with_x11_lib} directory doesn't contain libx11.])
      fi
    fi
  ])

  dnl If the header doesn't exist, there is no point looking for the library.
  if test x"${ac_cv_path_x11_incl}" != x ; then
    newlist="/Developer/SDKs/MacOSX10.6*.sdk/usr/lib /Developer/SDKs/MacOSX10.6*.sdk/usr/X11R6/lib /Developer/SDKs/MacOSX10.5*.sdk/usr/lib /Developer/SDKs/MacOSX10.5*.sdk/usr/X11R6/lib /Developer/SDKs/MacOSX10.4*.sdk/usr/lib /Developer/SDKs/MacOSX10.4*.sdk/usr/X11R6/lib ${libslist}"
    for i in $newlist; do
      if test -f $i/libX11.a -o -f $i/libX11.${shlibext}; then
        if test ! x"${i}" = x"/usr/lib" -a ! x"$i" = x"/usr/lib64"; then
          ac_cv_path_x11_lib="-L$i -lX11"
        else
          ac_cv_path_x11_lib="-lX11"
        fi
        if test -f $i/libXinerama.a -o -f $i/libXinerama.${shlibext}; then
          ac_cv_path_x11_lib="${ac_cv_path_x11_lib} -lXinerama"
        fi
        if test -f $i/libXext.a -o -f $i/libXext.${shlibext}; then
          ac_cv_path_x11_lib="${ac_cv_path_x11_lib} -lXext"
        fi
        if test -f $i/libSM.a -o -f $i/libSM.${shlibext}; then
          ac_cv_path_x11_lib="${ac_cv_path_x11_lib} -lSM"
        fi
        if test -f $i/libICE.a -o -f $i/libICE.${shlibext}; then
          ac_cv_path_x11_lib="${ac_cv_path_x11_lib} -lICE"
        fi
        break
      fi
    done
  fi

  for i in $newlist; do
    if test -f $i/libXplugin.a -o -f $i/libXplugin.${shlibext}; then
      if test ! x"${i}" = x"/usr/lib" -o x"$i" = x"/usr/lib64"; then
        ac_cv_path_x11_lib="${ac_cv_path_x11_lib} -L$i -lXplugin"
      else
        ac_cv_path_x11_lib="${ac_cv_path_x11_lib} -lXplugin"
      fi
      break
    fi
  done

  if test x"${ac_cv_path_x11_lib}" = x ; then
    AC_CHECK_LIB(X11, x11_mem_init, [ac_cv_path_x11_lib=""])
  fi

  AC_MSG_CHECKING([for X11 library])
  if test x"${ac_cv_path_x11_lib}" != x ; then
    X11_LIBS="${ac_cv_path_x11_lib}"
    AC_MSG_RESULT(${ac_cv_path_x11_lib})
    has_x11=yes
  else
    X11_LIBS=""
    AC_MSG_RESULT(none)
  fi

  if test "x$has_x11" = xyes; then
    AC_DEFINE(HAVE_X11, [1], [X11 headers and libraries])
  fi

  AC_SUBST(X11_CFLAGS)
  AC_SUBST(X11_LIBS)
])

# Local Variables:
# c-basic-offset: 2
# tab-width: 2
# indent-tabs-mode: nil
# End:
