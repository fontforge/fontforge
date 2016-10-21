/* Copyright (C) 2009-2012 by George Williams */
/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.

 * The name of the author may not be used to endorse or promote products
 * derived from this software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "fontforge.h"
#include "baseviews.h"
#include "libffstamp.h"			/* FontForge version# date time stamps */
#include "libstamp.h"
#include "uiinterface.h"

struct library_version_configuration library_version_configuration = {
    FONTFORGE_LIBFF_VERSION_MAJOR,
    FONTFORGE_LIBFF_VERSION_MINOR,
    LibFF_ModTime,			/* Seconds since 1970 (standard unix time) */
    LibFF_ModTime_Str,			/* Version date (in char string format)    */
    LibFF_VersionDate,			/* Version as long value, Year, month, day */
    sizeof(struct library_version_configuration),
    sizeof(struct splinefont),
    sizeof(struct splinechar),
    sizeof(struct fontviewbase),
    sizeof(struct charviewbase),
    sizeof(struct cvcontainer),
    1,
    1,

#ifdef _NO_PYTHON
    0,
#else
    1,
#endif
    0xff		/* Not currently defined */
};

/* Returns 0 on success, -1 for a fatal error, 1 for a dangerous condition */
int check_library_version(Library_Version_Configuration *exe_lib_version, int fatal, int quiet) {
    if (  exe_lib_version->major > library_version_configuration.major ||
	    ( exe_lib_version->major == library_version_configuration.major &&
	      exe_lib_version->minor > library_version_configuration.minor ) ||
	    exe_lib_version->sizeof_me != library_version_configuration.sizeof_me ||
	    exe_lib_version->sizeof_splinefont != library_version_configuration.sizeof_splinefont ||
	    exe_lib_version->sizeof_splinechar != library_version_configuration.sizeof_splinechar ||
	    exe_lib_version->sizeof_fvbase != library_version_configuration.sizeof_fvbase ||
	    exe_lib_version->sizeof_cvbase != library_version_configuration.sizeof_cvbase ||
	    exe_lib_version->sizeof_cvcontainer != library_version_configuration.sizeof_cvcontainer ||
	    exe_lib_version->config_had_devicetables != library_version_configuration.config_had_devicetables ||
	    exe_lib_version->config_had_multilayer != library_version_configuration.config_had_multilayer ||
	    exe_lib_version->config_had_python != library_version_configuration.config_had_python ||
	    exe_lib_version->mba1 != 0xff ) {
	if ( !quiet ) {
	    IError("This executable will not work with this version of libfontforge\nSee the console log for more details." );
	    if ( exe_lib_version->major > library_version_configuration.major ||
		    ( exe_lib_version->major == library_version_configuration.major &&
		      exe_lib_version->minor > library_version_configuration.minor ))
		fprintf( stderr, "Library version number is too small for executable.\n" );
	    if ( exe_lib_version->sizeof_me != library_version_configuration.sizeof_me )
		fprintf( stderr, "Configuration info in the executable has a different size than that\n  expected by the library and is not to be trusted.\n" );
	    if ( exe_lib_version->sizeof_splinefont != library_version_configuration.sizeof_splinefont )
		fprintf( stderr, "The internal data structure, SplineFont, has a different expected size\n  in library and executable. So they will not work together.\n" );
	    if ( exe_lib_version->sizeof_splinechar != library_version_configuration.sizeof_splinechar )
		fprintf( stderr, "The internal data structure, SplineChar, has a different expected size\n  in library and executable. So they will not work together.\n" );
	    if ( exe_lib_version->sizeof_fvbase != library_version_configuration.sizeof_fvbase )
		fprintf( stderr, "The internal data structure, FontViewBase, has a different expected size\n  in library and executable. So they will not work together.\n" );
	    if ( exe_lib_version->sizeof_cvbase != library_version_configuration.sizeof_cvbase )
		fprintf( stderr, "The internal data structure, CharViewBase, has a different expected size\n  in library and executable. So they will not work together.\n" );
	    if ( exe_lib_version->sizeof_cvcontainer != library_version_configuration.sizeof_cvcontainer )
		fprintf( stderr, "The internal data structure, CVContainer, has a different expected size\n  in library and executable. So they will not work together.\n" );
	    if ( exe_lib_version->config_had_devicetables != library_version_configuration.config_had_devicetables ) {
		if ( !exe_lib_version->config_had_devicetables )
		    fprintf( stderr, "The library is configured to support device tables, but the executable is\n  not. This may explain why data structures are of different sizes.\n" );
		else
		    fprintf( stderr, "The executable is configured to support device tables, but the library is\n  not. This may explain why data structures are of different sizes.\n" );
	    }
	    if ( exe_lib_version->config_had_multilayer != library_version_configuration.config_had_multilayer ) {
		if ( !exe_lib_version->config_had_multilayer )
		    fprintf( stderr, "The library is configured to support type3 font editing, but the executable is\n  not. This may explain why data structures are of different sizes.\n" );
		else
		    fprintf( stderr, "The executable is configured to support type3 font editing, but the library is\n  not. This may explain why data structures are of different sizes.\n" );
	    }
	    if ( exe_lib_version->config_had_python != library_version_configuration.config_had_python ) {
		if ( !exe_lib_version->config_had_python )
		    fprintf( stderr, "The library is configured to support python scripts, but the executable is\n  not. This may explain why data structures are of different sizes.\n" );
		else
		    fprintf( stderr, "The executable is configured to support python scripts, but the library is\n  not. This may explain why data structures are of different sizes.\n" );
	    }
	    if ( exe_lib_version->mba1 != 0xff )
		fprintf( stderr, "The executable specifies a configuration value the library does not expect.\n" );
	}
	if ( fatal )
exit( 1 );
	else
return( -1 );
    } else if ( exe_lib_version->library_source_modtime > library_version_configuration.library_source_modtime ) {
	if ( !quiet )
	    ff_post_notice(_("Library may be too old"),_("The library is older than the executable expects.\n   This might be ok.\nOr it may crash on you.\nYou have been warned." ));
return( 1 );
    } else
return( 0 );
}
