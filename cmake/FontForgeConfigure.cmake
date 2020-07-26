# Distributed under the original FontForge BSD 3-clause license

#[=======================================================================[.rst:
FontForgeConfigure
------------------

Sets up the required state to generate the config header for FontForge.

This could do with a lot of cleanup, especially in simplifying how some
of these defines work.

There are multiple definitions that are not covered by the config header,
but which are used throughout FontForge.

Defines that are not included, because they are obsolete, include:

_NO_LIBCAIRO

There are other defines where it is not clear if they should be
configured, or if they are defined locally in source only:

BIGICONS, CHUNKDEBUG, DEBUG, DEBUG_FREEHAND, ESCAPE_LIBXML_STRINGS,
FF_OVERLAP_VERBOSE, FF_RELATIONAL_GEOM, FLAG, GLYPH_DATA_DEBUG,
HANYANG, KNIFE_CONTINUOUS, KOREAN, MEMORY_MASK, MyMemory,
NATIVE_CALLBACKS, NEED_WIDE_CHAR, OPTIMIZE_TTF_INSTRS, THIRDS_IN_WIDTH,
UsingPThreads, _COMPOSITE_BROKEN, _DEBUGCRASHFONTFORGE, _WACOM_DRV_BROKEN

#]=======================================================================]

function(_set_negated outval inval)
  if(${inval})
    set(${outval} 0 PARENT_SCOPE)
  else()
    set(${outval} 1 PARENT_SCOPE)
  endif()
endfunction()

function(fontforge_generate_config template destination)
  include(CheckSymbolExists)
  include(CheckIncludeFile)
  include(TestBigEndian)

  # Platform specific checks
  test_big_endian(WORDS_BIGENDIAN)
  if(APPLE)
    set(_CursorsMustBe16x16 1)
    set(_Keyboard 1)
    set(__Mac 1)
  elseif(CYGWIN)
    set(__CygWin 1)
    set(_ModKeysAutoRepeat 1)
  endif()

  # Header checks
  check_include_file(execinfo.h HAVE_EXECINFO_H)
  check_include_file(ieeefp.h HAVE_IEEEFP_H)
  check_include_file(langinfo.h HAVE_LANGINFO_H)
  check_symbol_exists(nl_langinfo "langinfo.h" HAVE_NL_LANGINFO)

  # These are hard requirements/unsupported, should get rid of these
  set(HAVE_ICONV_H 1)
  set(HAVE_LIBINTL_H 1)
  set(_NO_LIBUNICODENAMES 1)

  # Configurable settings
  set(FONTFORGE_CONFIG_SHOW_RAW_POINTS ${ENABLE_DEBUG_RAW_POINTS})
  set(FONTFORGE_CONFIG_TILEPATH ${ENABLE_TILE_PATH})
  set(FONTFORGE_CONFIG_WRITE_PFM ${ENABLE_WRITE_PFM})
  if(REAL_TYPE STREQUAL "double")
    set(FONTFORGE_CONFIG_USE_DOUBLE 1)
  endif()
  if(ENABLE_FREETYPE_DEBUGGER) # this is a file path
    set(FREETYPE_HAS_DEBUGGER 1)
  endif()

  # Configurable features
  _set_negated(_NO_XKB "${X11_Xkb_FOUND}")
  _set_negated(_NO_XINPUT "${X11_Xi_FOUND}")

  if(NOT ENABLE_GUI OR NOT ENABLE_X11)
    set(X_DISPLAY_MISSING 1)
  endif()

  if(ENABLE_GUI AND NOT ENABLE_X11)
    set(FONTFORGE_CAN_USE_GDK 1)
  endif()

  set(FONTFORGE_CAN_USE_WOFF2 ${ENABLE_WOFF2_RESULT})

  _set_negated(_NO_FFSCRIPT "${ENABLE_NATIVE_SCRIPTING}")
  _set_negated(_NO_LIBJPEG "${ENABLE_LIBJPEG_RESULT}")
  _set_negated(_NO_LIBPNG "${ENABLE_LIBPNG_RESULT}")
  _set_negated(_NO_LIBSPIRO "${ENABLE_LIBSPIRO_RESULT}")
  _set_negated(_NO_LIBTIFF "${ENABLE_LIBTIFF_RESULT}")
  _set_negated(_NO_LIBUNGIF "${ENABLE_LIBGIF_RESULT}")
  _set_negated(_NO_LIBUNINAMESLIST "${ENABLE_LIBUNINAMESLIST_RESULT}")
  _set_negated(_NO_PYTHON "${ENABLE_PYTHON_SCRIPTING_RESULT}")
  _set_negated(_NO_LIBREADLINE "${ENABLE_LIBREADLINE_RESULT}")

  if(ENABLE_LIBSPIRO_RESULT)
    set(_LIBSPIRO_FUN ${Libspiro_FEATURE_LEVEL})
  endif()
  if(ENABLE_LIBUNINAMESLIST_RESULT)
    set(_LIBUNINAMESLIST_FUN ${Libuninameslist_FEATURE_LEVEL})
  endif()

  configure_file(${template} ${destination} ESCAPE_QUOTES @ONLY)
endfunction()

function(_get_git_version)
  if(NOT DEFINED FONTFORGE_GIT_VERSION)
    find_package(Git)
    if(Git_FOUND)
      execute_process(
        COMMAND
          "${GIT_EXECUTABLE}" "log" "--pretty=format:%H" "-n" "1"
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        RESULT_VARIABLE GIT_RETVAL
        OUTPUT_VARIABLE GIT_OUTPUT
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
      )
      if(${GIT_RETVAL} EQUAL 0)
        set(_git_version "${GIT_OUTPUT}")
      endif()
    endif()
    set(FONTFORGE_GIT_VERSION "${_git_version}" CACHE INTERNAL "Git revision that FontForge was built off")
  endif()
endfunction()

function(_get_modtime)
  if(NOT DEFINED FONTFORGE_MODTIME)
    set(_modtime "0")
    if(${CMAKE_VERSION} VERSION_LESS "3.6.0") # so unfortunate
      if(DEFINED ENV{SOURCE_DATE_EPOCH} AND "$ENV{SOURCE_DATE_EPOCH}" MATCHES "^[0-9]+$")
        set(_modtime "$ENV{SOURCE_DATE_EPOCH}")
      else()
        execute_process(
          COMMAND "date" "+%s"
          RESULT_VARIABLE DATE_RETVAL
          OUTPUT_VARIABLE DATE_OUTPUT
          ERROR_QUIET
          OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        if(${DATE_RETVAL} EQUAL 0 AND "${DATE_OUTPUT}" MATCHES "^[0-9]+$")
          set(_modtime "${DATE_OUTPUT}")
        endif()
      endif()
    else()
      string(TIMESTAMP _modtime "%s")
    endif()
    set(FONTFORGE_MODTIME "${_modtime}" CACHE INTERNAL "Unix epoch of when the build was initially configured")
  endif()
endfunction()

function(_get_modtime_str _modtime)
  if(NOT DEFINED FONTFORGE_MODTIME_STR)
    if(${CMAKE_VERSION} VERSION_LESS "3.6.0")
      execute_process(
        COMMAND "date" "-u" "--date=@${_modtime}" "+%Y-%m-%d %H:%M UTC"
        RESULT_VARIABLE DATE_RETVAL
        OUTPUT_VARIABLE DATE_OUTPUT
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
      )
      if(${DATE_RETVAL} EQUAL 0)
        set(_modtime_str "${DATE_OUTPUT}")
      endif()
    else()
      string(TIMESTAMP _modtime_str "%Y-%m-%d %H:%M UTC" UTC)
    endif()
    set(FONTFORGE_MODTIME_STR "${_modtime_str}" CACHE INTERNAL "Human readable string of when the build was initially configured")
  endif()
endfunction()

function(fontforge_generate_version_extras template destination)
    _get_git_version()
    _get_modtime()
    _get_modtime_str(${FONTFORGE_MODTIME})

    configure_file(${template} ${destination} ESCAPE_QUOTES @ONLY)
endfunction()
