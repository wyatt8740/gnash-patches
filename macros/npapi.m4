dnl  
dnl    Copyright (C) 2005, 2006, 2009, 2010, 2011 Free Software Foundation, Inc.
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



AC_DEFUN([GNASH_PATH_NPAPI],
[

  dnl Look for npapi.h
  AC_ARG_WITH(npapi_incl, AS_HELP_STRING([--with-npapi-incl], [directory where npapi headers are]), with_npapi_incl=${withval})
    AC_CACHE_VAL(ac_cv_path_npapi_incl,[
    if test x"${with_npapi_incl}" != x ; then
      if test -f ${with_npapi_incl}/npapi.h; then
        ac_cv_path_npapi_incl="`(cd ${with_npapi_incl}; pwd)`"
      else
      	AC_MSG_ERROR([${with_npapi_incl} directory doesn't contain npapi.h])
      fi
    fi
  ])


  if test x$cross_compiling = xno; then
    if test x"$PKG_CONFIG" != x -a x"${ac_cv_path_npapi_incl}" = x; then
      if $PKG_CONFIG --exists npapi-sdk; then
        NPAPI_CFLAGS="`$PKG_CONFIG --cflags npapi-sdk`"
      elif $PKG_CONFIG --exists mozilla-plugin; then
        NPAPI_CFLAGS="`$PKG_CONFIG --cflags mozilla-plugin`"
      fi
    fi
  fi

  dnl Attempt to find the top level directory, which unfortunately has a
  dnl version number attached. At least on Debain based systems, this
  dnl doesn't seem to get a directory that is unversioned.

  AC_MSG_CHECKING([for npapi.h])
  if test x"${ac_cv_path_npapi_incl}" = x; then
    gnash_npapi_topdir=""
    for i in $incllist "/usr/local"; do
      for j in `ls -dr $i/xulrunner* 2>/dev/null`; do
        if test -f $j/npapi.h; then
          ac_cv_path_npapi_incl="${j}"
          break 2
        fi
        if test -f $j/plugin/npapi.h; then
          ac_cv_path_npapi_incl="${j}/plugin"
          break 2
        fi
        if test -f $j/include/npapi.h; then
          ac_cv_path_npapi_incl="${j}/include"
          break 2
        fi
        if test -f $j/include/plugin/npapi.h; then
          ac_cv_path_npapi_incl="${j}/include/plugin"
          break 2
        fi
      done
    done
  fi
  if test x"${ac_cv_path_npapi_incl}" = x -a x"${NPAPI_CFLAGS}" = x; then
    AC_MSG_RESULT([not found])
    has_npapi=no
  elif test x"${ac_cv_path_npapi_incl}" = x -a x"${NPAPI_CFLAGS}" != x; then
    AC_MSG_RESULT([${NPAPI_CFLAGS}])
    has_npapi=yes
  else
    NPAPI_CFLAGS="-I${ac_cv_path_npapi_incl}"
    if test x"$linux" = x"yes" -o x"$bsd" = x"yes" -o x"$solaris" = x"yes";then
        NPAPI_CFLAGS="$NPAPI_CFLAGS -DXP_UNIX"
    fi
    AC_MSG_RESULT([${NPAPI_CFLAGS}])
    has_npapi=yes
  fi

  if test x"${NPAPI_CFLAGS}" != x; then
    AC_LANG_PUSH(C++)
    save_CXXFLAGS="$CXXFLAGS"
    CXXFLAGS="$CXXFLAGS $NPAPI_CFLAGS"
    AC_MSG_CHECKING([whether NPString has member UTF8Length])
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([
                  #include "npapi.h" 
                  #include "npruntime.h"
                  int main(int argc, char** argv){
	            NPString str;
                    uint32_t len = str.UTF8Length;
	            return 0;
                  }])],
                 [AC_DEFINE([NPAPI_VERSION],[192],[Define that we have NPAPI present in version 1.9.2 and newer])
                  AC_MSG_RESULT([yes])],
                 [AC_MSG_RESULT([no])])
    CXXFLAGS="$save_CXXFLAGS"
    AC_LANG_POP(C++)
    AC_MSG_CHECKING([for npupp.h])
    if test -f "${ac_cv_path_npapi_incl}"/npupp.h -o \
            -f "${ac_cv_path_npapi_incl}"/plugin/npupp.h -o \
            -f "`$PKG_CONFIG --variable=includedir mozilla-plugin`"/npupp.h -o \
            -f "`$PKG_CONFIG --variable=includedir mozilla-plugin`"/stable/npupp.h -o \
            -f "`$PKG_CONFIG --variable=includedir mozilla-plugin`"/plugin/npupp.h;then
      AC_DEFINE([NPAPI_VERSION],[190],[Define that we have NPAPI present in version 1.9.0 and maybe older])
      AC_MSG_RESULT([yes])
    else
      AC_MSG_RESULT([no])
    fi
  fi

  AC_SUBST(NPAPI_CFLAGS)
])

# Local Variables:
# c-basic-offset: 2
# tab-width: 2
# indent-tabs-mode: nil
# End:
