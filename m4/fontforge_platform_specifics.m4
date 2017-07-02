dnl -*- autoconf -*-

dnl FONTFORGE_PLATFORM_SPECIFICS
dnl ----------------------------
AC_DEFUN([FONTFORGE_PLATFORM_SPECIFICS],
[
AC_CANONICAL_HOST

m4_define([default_SDK],[/])
m4_define([default_CARBON],[System/Library/Frameworks/Carbon.framework/Carbon])
m4_define([default_COCOA],[/System/Library/Frameworks/Cocoa.framework/Cocoa])
m4_define([default_CORESERVICES],[System/Library/Frameworks/CoreServices.framework/CoreServices])

AS_CASE([$host],
   [*-apple-darwin*],[
      gww_ismac="yes"
      AC_DEFINE([_CursorsMustBe16x16],[1],[Platform specific stuff])
      AC_DEFINE([_Keyboard],[1],[Platform specific stuff])
      AC_DEFINE([__Mac],[1],[Platform specific stuff])
      MACAPP=FontForge.app/Contents/Info.plist
      gww_define_caps_for_alt="1"
      if test "$no_x" = "yes"; then
       MACAPP=""
      fi

      RAW_COMPILE_PLATFORM_CFLAGS=" $CFLAGS "
      AC_SUBST([RAW_COMPILE_PLATFORM_CFLAGS])

      dnl fink puts stuff under /sw
      dnl macports puts stuff under /opt/local
      dnl but when macport/fink overwrite a standard lib, I still want the standard
      dnl  library to be found (otherwise only portable to machines with macports/fink)

      if test "$CPPFLAGS" \!= "" >/dev/null ; then
       TrustUser="yes"
      elif test -d /sw/include >/dev/null ; then
       CPPFLAGS="$CPPFLAGS -I /sw/include"
       gww_extraincludes="/sw/include"
      elif test -d /opt/local/include >/dev/null ; then
       CPPFLAGS="$CPPFLAGS -I /opt/local/include"
       gww_extraincludes="/opt/local/include";
      fi
      if test "$oldLDFLAGS" \!= "" >/dev/null ; then
       TrustUser="yes"
      elif test -d /sw/lib >/dev/null ; then
       LDFLAGS="$LDFLAGS -L/sw/lib"
       gww_rpath="-rpath /sw/lib"
      elif test -d /opt/local/lib >/dev/null ; then
       LDFLAGS="$LDFLAGS -L/opt/local/lib"
       gww_rpath="-rpath /opt/local/lib"
      fi

      AC_ARG_VAR([SDK],[(Macintosh only) path to software development kit; defaults to ]default_SDK)
      AC_ARG_VAR([CARBON],[(Macintosh only) path to Carbon; defaults to SDK/]default_CARBON)
      AC_ARG_VAR([COCOA],[(Macintosh only) path to Cocoa; defaults to SDK/]default_COCOA)
      AC_ARG_VAR([CORESERVICES],[(Macintosh only) path to CoreServices; defaults to SDK/]default_CORESERVICES)

      test x"${SDK}" = x && SDK="default_SDK"
      if test x"${SDK}" = x"/"; then
         derooted_SDK=""
      else
         derooted_SDK="${SDK}"
      fi
      test x"${CARBON}" = x && CARBON="${derooted_SDK}/default_CARBON"
      test x"${COCOA}" = x && CARBON="${derooted_SDK}/default_COCOA"
      test x"${CORESERVICES}" = x && CORESERVICES="${derooted_SDK}/default_CORESERVICES"

      if test x"${cross_compiling}" != xyes; then
         test x"${CORESERVICES}" != x && LIBS="-Wl,${CORESERVICES} ${LIBS}"
         test x"${CARBON}" != x && LIBS="-Wl,${CARBON} ${LIBS}"
         test x"${COCOA}" != x && LIBS="-Wl,${COCOA} ${LIBS}"
         # I would like to be able to just leave the framework as a
         # filename. But if I do that, libtool eats it and gcc doesn't
         # get it and ld doesn't get it and we get undefined symbols.
      fi
   ],

   [powerpc-*-*linux*],[AC_DEFINE([_Keyboard],[3])],

   [*sparc*],[AC_DEFINE([_Keyboard],[2])],

   [*mingw*],[ LDFLAGS="$LDFLAGS -lws2_32" ],

   [*-pc-cygwin*],[

      AC_DEFINE([_BrokenBitmapImages],[1],[Platform specific stuff])
      AC_DEFINE([_ModKeysAutoRepeat],[1],[Platform specific stuff])
      AC_DEFINE([__CygWin],[1],[Platform specific stuff])

      gww_iscygwin="yes"
   ],

   [:]  dnl DEFAULT AS_CASE

) dnl END AS_CASE

AM_CONDITIONAL([PLATFORM_OSX],[test x"${gww_ismac}" = xyes])

])
